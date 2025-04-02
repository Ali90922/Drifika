#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <ucontext.h>
#include <signal.h>
#include <time.h> // Only needed if you want time-based MLFQ
#include "nqp_thread.h"
#include "nqp_thread_sched.h"

#define MAX_THREADS 50

/**
 * Struct representing an NQP thread.
 * For MLFQ, you might add fields like priority, time-slice usage, etc.
 */
typedef struct nqp_thread_t
{
    ucontext_t context;
    void *stack;
    int finished;
    int id; // Optional unique ID for debugging
} nqp_thread_t;

/** Global array of threads */
static nqp_thread_t *thread_queue[MAX_THREADS];
static int num_threads = 0;
static int current_index = 0;
static nqp_thread_t *current_thread = NULL;

/** Global policy */
static nqp_scheduling_policy system_policy = NQP_SP_TWOTHREADS;

// -----------------------------------------------------------------
// Helper: Add a thread to the global queue
// -----------------------------------------------------------------
void scheduler_add_thread(nqp_thread_t *thread)
{
    if (num_threads < MAX_THREADS)
    {
        thread_queue[num_threads++] = thread;
    }
    else
    {
        fprintf(stderr, "Exceeded maximum thread capacity!\n");
        exit(EXIT_FAILURE);
    }
}

// -----------------------------------------------------------------
// Thread wrapper: calls the user function, marks finished, exits
// -----------------------------------------------------------------
void thread_wrapper(void (*task)(void *), void *arg, nqp_thread_t *thread)
{
    task(arg);            // Run the user function
    thread->finished = 1; // Mark finished
    nqp_exit();           // Yield (and schedule next thread)
    // never returns
}

// -----------------------------------------------------------------
// nqp_thread_create: Allocates and initializes a thread
// -----------------------------------------------------------------
nqp_thread_t *nqp_thread_create(void (*task)(void *), void *arg)
{
    assert(task != NULL);

    nqp_thread_t *new_thread = malloc(sizeof(nqp_thread_t));
    if (!new_thread)
    {
        return NULL;
    }

    new_thread->stack = malloc(SIGSTKSZ);
    if (!new_thread->stack)
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
    new_thread->id = num_threads; // optional ID

    new_thread->context.uc_stack.ss_sp = new_thread->stack;
    new_thread->context.uc_stack.ss_size = SIGSTKSZ;
    new_thread->context.uc_stack.ss_flags = 0;
    new_thread->context.uc_link = NULL; // No next context

    // Build a context that calls thread_wrapper(task, arg, new_thread)
    makecontext(&new_thread->context,
                (void (*)(void))thread_wrapper,
                3,
                task,
                arg,
                new_thread);

    scheduler_add_thread(new_thread);
    return new_thread;
}

// -----------------------------------------------------------------
// nqp_thread_join: busy-wait by yielding until the given thread finishes
// -----------------------------------------------------------------
int nqp_thread_join(nqp_thread_t *thread)
{
    assert(thread != NULL);

    while (!thread->finished)
    {
        nqp_yield();
    }
    return 0; // successfully joined
}

// -----------------------------------------------------------------
// nqp_exit: Mark current thread finished, yield to next
// -----------------------------------------------------------------
void nqp_exit(void)
{
    if (current_thread)
    {
        current_thread->finished = 1;
    }
    nqp_yield();

    // If we get here, no runnable thread was found:
    // we can safely exit the process or just return to main.
    exit(0);
}

// -----------------------------------------------------------------
// Scheduling policy set/init
// -----------------------------------------------------------------
int nqp_sched_init(const nqp_scheduling_policy policy,
                   const nqp_sp_settings *settings)
{
    // For MLFQ expansions, you'd process 'settings' here.
    (void)settings;

    assert(policy >= NQP_SP_TWOTHREADS && policy < NQP_SP_POLICIES);
    if (policy >= NQP_SP_TWOTHREADS && policy < NQP_SP_POLICIES)
    {
        system_policy = policy;
        return 0;
    }
    return -1;
}

// -----------------------------------------------------------------
// any_other_thread_unfinished: returns 1 if there's an unfinished thread
//                              other than 'ignore'.
// -----------------------------------------------------------------
static int any_other_thread_unfinished(nqp_thread_t *ignore)
{
    for (int i = 0; i < num_threads; i++)
    {
        if (!thread_queue[i]->finished && thread_queue[i] != ignore)
        {
            return 1;
        }
    }
    return 0;
}

// -----------------------------------------------------------------
// FIFO yield: no-op if still running. If called from nqp_exit, pick next thread
// -----------------------------------------------------------------
static void fifo_yield(void)
{
    // If the thread is still running, do nothing.
    if (!current_thread->finished)
    {
        return;
    }
    // If the thread just finished, find the next unfinished thread.
    if (!any_other_thread_unfinished(current_thread))
    {
        // None left, we return to sched_start eventually
        return;
    }
    // find the first unfinished
    for (int i = 0; i < num_threads; i++)
    {
        if (!thread_queue[i]->finished)
        {
            nqp_thread_t *next = thread_queue[i];
            nqp_thread_t *prev = current_thread;
            current_index = i;
            current_thread = next;
            swapcontext(&prev->context, &next->context);
            break;
        }
    }
}

// -----------------------------------------------------------------
// Round-robin yield
// -----------------------------------------------------------------
static void rr_yield(void)
{
    int next_index = (current_index + 1) % num_threads;
    nqp_thread_t *next = thread_queue[next_index];

    int iterations = 0;
    while (next->finished && iterations < num_threads)
    {
        next_index = (next_index + 1) % num_threads;
        next = thread_queue[next_index];
        iterations++;
    }
    // If no other thread is available, do nothing
    if (next == current_thread)
    {
        return;
    }
    nqp_thread_t *prev = current_thread;
    current_index = next_index;
    current_thread = next;
    swapcontext(&prev->context, &next->context);
}

// -----------------------------------------------------------------
// MLFQ yield (placeholder).
// In a real MLFQ, you'd track time usage & queue levels, etc.
// -----------------------------------------------------------------
static void mlfq_yield(void)
{
    // For real MLFQ, you'd measure CPU usage of current_thread, demote if needed,
    // or check if a boost is due. For now, we mimic round-robin among all threads.
    rr_yield();
}

// -----------------------------------------------------------------
// nqp_yield: calls the right yield function depending on system_policy
// -----------------------------------------------------------------
void nqp_yield(void)
{
    if (!current_thread)
    {
        return; // not in a thread context
    }

    switch (system_policy)
    {
    case NQP_SP_TWOTHREADS:
    case NQP_SP_RR:
        rr_yield();
        break;

    case NQP_SP_FIFO:
        fifo_yield();
        break;

    case NQP_SP_MLFQ:
        mlfq_yield();
        break;

    default:
        // fallback: do nothing
        break;
    }
}

// -----------------------------------------------------------------
// nqp_sched_start:
//    TWOTHREADS: never returns (ctrl+c to kill)
//    Others: single swap into the first thread.
//            When done, returns to main() so you can join or exit.
// -----------------------------------------------------------------
void nqp_sched_start(void)
{
    if (num_threads == 0)
    {
        return;
    }

    ucontext_t main_context;

    // NQP_SP_TWOTHREADS => switch once, never return
    if (system_policy == NQP_SP_TWOTHREADS)
    {
        current_index = 0;
        current_thread = thread_queue[current_index];
        swapcontext(&main_context, &current_thread->context);
        // never returns; user kills program with ctrl+c
    }
    else
    {
        // FIFO, RR, MLFQ => do one initial swap. yields handle the rest.
        current_index = 0;
        current_thread = thread_queue[current_index];
        swapcontext(&main_context, &current_thread->context);

        // Once all threads finish, we come back here & return to main.
        // main can then do any final cleanup or just exit.
    }
}
