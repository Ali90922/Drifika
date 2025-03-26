#include <stdlib.h>
#include <assert.h>
#include <ucontext.h>
#include <signal.h>

#include "nqp_thread.h"
#include "nqp_thread_sched.h"
// For simplicity, assume we have a fixed-size thread queue.

#define MAX_THREADS 10

// Global thread queue and scheduling index.
static nqp_thread_t *thread_queue[MAX_THREADS];
static int num_threads = 0;
static int current_index = 0;

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

nqp_thread_t *nqp_thread_create(void (*task)(void *), void *arg)
{
    assert(task != NULL);

    // Allocate memory for the thread control block.
    nqp_thread_t *new_thread = malloc(sizeof(nqp_thread_t));
    if (new_thread == NULL)
    {
        return NULL;
    }

    // Allocate a new stack for the thread.
    new_thread->stack = malloc(SIGSTKSZ);
    if (new_thread->stack == NULL)
    {
        free(new_thread);
        return NULL;
    }

    // Initialize the thread's context.
    if (getcontext(&new_thread->context) == -1)
    {
        free(new_thread->stack);
        free(new_thread);
        return NULL;
    }

    // Initialize the finished flag.
    new_thread->finished = 0;

    // Set up the stack for the new context.
    new_thread->context.uc_stack.ss_sp = new_thread->stack;
    new_thread->context.uc_stack.ss_size = SIGSTKSZ;
    new_thread->context.uc_stack.ss_flags = 0;
    new_thread->context.uc_link = NULL;

    // Instead of running 'task' directly, run thread_wrapper.
    // The '3' indicates that three arguments will be passed.
    makecontext(&new_thread->context, (void (*)(void))thread_wrapper, 3, task, arg, new_thread);

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
    // schedule another (maybe different) task.
}

void nqp_exit(void)
{
    // remove the currently executing thread from the system.
}

void nqp_sched_start(void)
{
    // start scheduling tasks.
}
