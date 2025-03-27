#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <ucontext.h>
#include <signal.h>

#include "nqp_thread.h"
#include "nqp_thread_sched.h"
// For simplicity, assume we have a fixed-size thread queue.

#define MAX_THREADS 40

// Queue Datastructures to hold the threads :

// Global scheduler queue: holds pointers to all NQP threads that are runnable.
static nqp_thread_t *thread_queue[MAX_THREADS];
static int num_threads = 0;   // How many threads have been added to the scheduler.
static int current_index = 0; // Index of the currently running thread.

// Global pointer to the currently running thread.
static nqp_thread_t *current_thread = NULL;

typedef struct nqp_thread_t
{
    ucontext_t context;
    void *stack;
    int finished; // 0 means running, 1 means finished
    // Optionally, add thread ID, status, priority, etc.
    int id; // Unique identifier for the thread.
} nqp_thread_t;

static nqp_scheduling_policy system_policy = NQP_SP_TWOTHREADS;

/**
 * Initialize an nqp_thread_t.
 *
 * Args:
 *  task: the function that this thread should begin executing in. Must not be
 *        NULL.
 *  arg: the arguments to be passed to the function. May be NULL if the function
 *       does not actually use the arguments passed to it.
 * Return: An initialized thread or NULL on error.
 */

// nqp_thread_create creates and intialises and returns  a Thread control block that is ready to be scheduled
nqp_thread_t *nqp_thread_create(void (*task)(void *), void *arg)
{
    assert(task != NULL);
    nqp_thread_t *new_thread = malloc(sizeof(nqp_thread_t));
    if (new_thread == NULL)
        return NULL;

    new_thread->stack = malloc(SIGSTKSZ);
    if (new_thread->stack == NULL)
    {
        free(new_thread);
        return NULL;
    }

    if (getcontext(&new_thread->context) == -1)
    {
        free(new_thread->stack);
        free(new_thread);
        return NULL;
    }

    new_thread->finished = 0;
    // (Optional) You already have an 'id' field; you might assign it here.

    new_thread->context.uc_stack.ss_sp = new_thread->stack;
    new_thread->context.uc_stack.ss_size = SIGSTKSZ;
    new_thread->context.uc_stack.ss_flags = 0;
    new_thread->context.uc_link = NULL;

    // Use thread_wrapper to run the task, mark finished, and exit.
    makecontext(&new_thread->context, (void (*)(void))thread_wrapper, 3, task, arg, new_thread);

    // Add the new thread to the scheduler's queue.
    scheduler_add_thread(new_thread);

    return new_thread;
}

// Helper Function -- for the Done Flag
void thread_wrapper(void (*task)(void *), void *arg, nqp_thread_t *thread)
{
    // Execute the user-provided function.
    task(arg);
    // Mark the thread as finished.
    thread->finished = 1;
    // Terminate the thread (and remove it from the scheduler).
    nqp_exit();
}

// Helper Function to add thread to the global queue
void scheduler_add_thread(nqp_thread_t *thread)
{
    if (num_threads < MAX_THREADS)
    {
        thread_queue[num_threads++] = thread;
    }
    else
    {
        // Handle error: exceeded maximum threads (or dynamically grow the queue)
        fprintf(stderr, "Exceeded maximum thread capacity!\n");
        exit(EXIT_FAILURE);
    }
}

/**
 * Wait for the specified thread to finish. This function will block the caller
 * until the specified thread is finished its current task.
 *
 * Args:
 *  thread: the thread to wait for. Must not be NULL. Must have been previously
 *          initialized.
 * Return: 0 on success (e.g., the thread has exited), -1 on failure.
 */

int nqp_thread_join(nqp_thread_t *thread)
{
    assert(thread != NULL);

    // Busy-wait (with yielding) until the thread is finished.
    while (!thread->finished)
    {
        nqp_yield();
    }

    return 0; // Successfully joined.
}

// 4 Fucntions from nqp_thread_sched.h !

/**
 * The policy that should be used when deciding which task to switch to next
 * upon a call to yield.
 *
 * Args:
 *  policy: the policy to apply. Must be a policy as defined in
 *          NQP_SCHEDULING_POLICY.
 *  settings: The settings for the policy to apply. May be NULL, depending on
 *            the policy being set. See nqp_scheduling_policy.
 * Returns: 0 on success, -1 on failure (e.g., the specified policy is not in
 *          NQP_SCHEDULING_POLICY).
 */

int nqp_sched_init(const nqp_scheduling_policy policy,
                   const nqp_sp_settings *settings)
{
    int ret = -1;
    (void)settings;

    assert(policy >= NQP_SP_TWOTHREADS && policy < NQP_SP_POLICIES);

    if (policy >= NQP_SP_TWOTHREADS && policy < NQP_SP_POLICIES)
    {
        system_policy = policy;
        ret = 0;
    }
    return ret;
}

/**
 * Voluntarily give up control of the processor and allow the scheduler to
 * schedule another task.
 *
 * When called outside of the context of an NQP thread (e.g., nqp_thread_start
 * has not been called), this function should have no side-effects at all (the
 * function should behave as a no-op).
 */
void nqp_yield(void)
{
    // If not running inside an NQP thread, do nothing.
    if (current_thread == NULL)
        return;

    nqp_thread_t *prev = current_thread;

    // Round-robin: advance current_index modulo the number of threads.
    int next_index = (current_index + 1) % num_threads;
    nqp_thread_t *next = thread_queue[next_index];

    // Skip finished threads. (This simple loop stops if it circles back to the current thread.)
    int iterations = 0;
    while (next->finished && iterations < num_threads)
    {
        next_index = (next_index + 1) % num_threads;
        next = thread_queue[next_index];
        iterations++;
    }
    // If all threads are finished (or the next is still the current one), do nothing.
    if (next == prev)
        return;

    current_index = next_index;
    current_thread = next;

    // Swap contexts: save current state and switch to next thread.
    swapcontext(&prev->context, &next->context);
}

/**
 * The current task is done, it should be removed from the queue of tasks to
 * be scheduled.
 *
 * When called outside of the context of an NQP thread (e.g., nqp_thread_start
 * has not been called), this function should have no side-effects at all (the
 * function should behave as a no-op).
 */
void nqp_exit(void)
{
    // Mark the current thread as finished.
    if (current_thread != NULL)
        current_thread->finished = 1;

    // Yield control to allow the scheduler to pick another thread.
    nqp_yield();

    // In a well-designed system, execution should never return here.
    // If it does, exit the process.
    exit(0);
}

/**
 * Start scheduling tasks.
 *
 * If the NQP_SP_TWOTHREADS policy is selected, control flow will never return
 * to the caller (the program must be terminated by sending the SIGINT signal).
 *
 * If any other scheduling policy is selected, control flow will eventually
 * return to the caller. The caller is then responsible for blocking itself
 * until all threads of execution that it has started have finished by calling
 * nqp_thread_join for each thread it has started.
 */
void nqp_sched_start(void)
{
    // start scheduling tasks.

    // Need if else statements with :   system_policy
    /*
    For NQP_SP_TWOTHREADS: Swap between the two threads.
    For NQP_SP_FIFO: Always run the next thread in FIFO order.
    For NQP_SP_RR: Use round-robin scheduling.
    For NQP_SP_MLFQ: Use multiple queues and adjust priorities
    */

    if (num_threads == 0)
        return;

    current_index = 0;
    current_thread = thread_queue[current_index];

    // This swapcontext never returns unless all threads call nqp_exit and no thread is runnable.
    ucontext_t main_context;
    swapcontext(&main_context, &current_thread->context);

    // Control will only reach here if the scheduler is designed to eventually return.
}
