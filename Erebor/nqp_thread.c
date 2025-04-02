#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <ucontext.h>
#include <signal.h>

#include "nqp_thread.h"
#include "nqp_thread_sched.h"

#define MAX_THREADS 50

/*
 * The nqp_thread_t struct.
 * (You can also place this in nqp_thread.h if needed, but it's typically
 * private to the .c file. Same for the prototypes below.)
 */
typedef struct nqp_thread_t
{
    ucontext_t context;
    void *stack;
    int finished;
    int id; // Possibly unused, but that's fine.
} nqp_thread_t;

/* Global scheduler queue */
static nqp_thread_t *thread_queue[MAX_THREADS];
static int num_threads = 0;
static int current_index = 0;
static nqp_thread_t *current_thread = NULL;

/* The selected scheduling policy, default to NQP_SP_TWOTHREADS */
static nqp_scheduling_policy system_policy = NQP_SP_TWOTHREADS;

/* Prototypes for internal helpers */
void thread_wrapper(void (*task)(void *), void *arg, nqp_thread_t *thread);
void scheduler_add_thread(nqp_thread_t *thread);

/* -------------------------------------------------------------------------
 * Thread creation and wrapper
 * ------------------------------------------------------------------------- */
nqp_thread_t *nqp_thread_create(void (*task)(void *), void *arg)
{
    assert(task != NULL);

    nqp_thread_t *new_thread = malloc(sizeof(nqp_thread_t));
    if (!new_thread)
        return NULL;

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

    new_thread->context.uc_stack.ss_sp = new_thread->stack;
    new_thread->context.uc_stack.ss_size = SIGSTKSZ;
    new_thread->context.uc_stack.ss_flags = 0;
    new_thread->context.uc_link = NULL;

    // Wrap the user function in thread_wrapper
    makecontext(&new_thread->context,
                (void (*)(void))thread_wrapper,
                3,
                task,
                arg,
                new_thread);

    // Add the new thread to the scheduler queue
    scheduler_add_thread(new_thread);

    return new_thread;
}

/* The actual function that each "thread" runs first.
 * Once `task(arg)` completes, we mark finished and call nqp_exit().
 */
void thread_wrapper(void (*task)(void *), void *arg, nqp_thread_t *thread)
{
    task(arg);
    thread->finished = 1;
    nqp_exit();
}

/* Add thread to the global queue */
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

/* -------------------------------------------------------------------------
 * Thread join
 * ------------------------------------------------------------------------- */
int nqp_thread_join(nqp_thread_t *thread)
{
    assert(thread != NULL);

    /* Busy-wait with yield until the thread is finished.
     * The actual scheduling for FIFO / RR / MLFQ is in nqp_sched_start(),
     * but if we are running in two-thread mode, yield *does* flip threads.
     */
    while (!thread->finished)
    {
        nqp_yield();
    }
    return 0;
}

/* -------------------------------------------------------------------------
 * Scheduling API
 * ------------------------------------------------------------------------- */
int nqp_sched_init(const nqp_scheduling_policy policy, const nqp_sp_settings *settings)
{
    (void)settings;

    if (policy < NQP_SP_TWOTHREADS || policy >= NQP_SP_POLICIES)
        return -1;

    system_policy = policy;
    return 0;
}

/*
 * nqp_yield:
 *    - If we're in TWO-THREAD mode, do the swap to the other thread (round-robin of 2).
 *    - If FIFO / RR / MLFQ, do nothing except maybe let the OS run something else.
 *      The "which thread runs next" logic is in nqp_sched_start's while loop for these policies.
 */
void nqp_yield(void)
{
    /* If we haven't even started scheduling (no current_thread), do nothing. */
    if (!current_thread)
        return;

    /* For FIFO, do nothing. The current thread remains the same until it finishes. */
    if (system_policy == NQP_SP_FIFO)
    {
        return;
    }

    /* If we are in two-thread mode, handle the yield by flipping to the other thread. */
    if (system_policy == NQP_SP_TWOTHREADS)
    {
        nqp_thread_t *prev = current_thread;

        /* Move to next index (just 2 threads assumed, but if >2, we'll do simple round-robin) */
        int next_index = (current_index + 1) % num_threads;
        nqp_thread_t *next = thread_queue[next_index];

        /* Skip finished threads if possible. If all finished except us, do nothing. */
        int iterations = 0;
        while (next->finished && iterations < num_threads)
        {
            next_index = (next_index + 1) % num_threads;
            next = thread_queue[next_index];
            iterations++;
        }
        // If there's no other valid thread to run (or next == prev and it's finished), just return.
        if (next == prev || next->finished)
            return;

        current_index = next_index;
        current_thread = next;
        swapcontext(&prev->context, &next->context);
        return;
    }

    /* If Round-Robin or MLFQ, we also do nothing in yield
     * because the big loop in nqp_sched_start is controlling scheduling.
     */
    if (system_policy == NQP_SP_RR || system_policy == NQP_SP_MLFQ)
    {
        return;
    }

    /* Default fallback: do nothing. */
}

/*
 * nqp_exit:
 *   Mark current thread finished, then yield.
 *   If two-thread mode, yield flips to the other.
 *   If other mode, no effect except that next time we get back to nqp_sched_start loop.
 */
void nqp_exit(void)
{
    if (current_thread)
        current_thread->finished = 1;

    /* Yield to let something else run.
     * In two-thread mode, we do context switch now.
     * In the other modes, nqp_sched_start’s loop picks who runs next.
     */
    nqp_yield();

    /* If we return here, let's forcibly exit to avoid confusion. */
    exit(0);
}

/*
 * nqp_sched_start:
 *   In two-thread mode: we just do one swapcontext to the first thread.
 *     All subsequent scheduling is done by nqp_yield().
 *   In FIFO/RR/MLFQ: we have a big loop that picks which thread to run next until all are finished.
 */
void nqp_sched_start(void)
{
    if (num_threads == 0)
        return;

    ucontext_t main_context;

    switch (system_policy)
    {
    case NQP_SP_TWOTHREADS:
        /* Two-thread mode: swap to the first thread; yield handles switching. */
        current_index = 0;
        current_thread = thread_queue[current_index];
        swapcontext(&main_context, &current_thread->context);
        /* Control never returns until both are done or main thread calls nqp_thread_join. */
        break;

    case NQP_SP_FIFO:
        /* FIFO: run threads in order, never switching until they finish. */
        while (1)
        {
            nqp_thread_t *next = NULL;
            for (int i = 0; i < num_threads; i++)
            {
                if (!thread_queue[i]->finished)
                {
                    next = thread_queue[i];
                    break;
                }
            }
            if (!next)
                break; // all finished

            current_thread = next;
            swapcontext(&main_context, &current_thread->context);
            // Return here if next called yield/exit. Loop picks it again if it's not finished.
        }
        break;

    case NQP_SP_RR:
        /* Round-Robin: a loop that cycles through threads, ignoring those finished. */
        current_index = 0;
        while (1)
        {
            int any_unfinished = 0;
            for (int i = 0; i < num_threads; i++)
            {
                if (!thread_queue[i]->finished)
                {
                    any_unfinished = 1;
                    break;
                }
            }
            if (!any_unfinished)
                break; // done

            // Move to next thread
            current_index = (current_index + 1) % num_threads;
            while (thread_queue[current_index]->finished)
                current_index = (current_index + 1) % num_threads;

            current_thread = thread_queue[current_index];
            swapcontext(&main_context, &current_thread->context);
        }
        break;

    case NQP_SP_MLFQ:
    {
/* MLFQ: maintain multiple levels of queues. */
#define NUM_QUEUES 3
        nqp_thread_t *queues[NUM_QUEUES][MAX_THREADS] = {0};
        int queue_sizes[NUM_QUEUES] = {0};
        int rr_index[NUM_QUEUES] = {0};

        // Put all threads in top queue initially
        for (int i = 0; i < num_threads; i++)
        {
            queues[0][queue_sizes[0]++] = thread_queue[i];
        }

        while (1)
        {
            // Check if all done
            int all_done = 1;
            for (int i = 0; i < num_threads; i++)
            {
                if (!thread_queue[i]->finished)
                {
                    all_done = 0;
                    break;
                }
            }
            if (all_done)
                break;

            // Find highest non-empty queue
            int q = -1;
            for (int i = 0; i < NUM_QUEUES; i++)
            {
                if (queue_sizes[i] > 0)
                {
                    q = i;
                    break;
                }
            }
            if (q == -1)
                break; // something's off or all done

            // Round-robin pick from queue q
            nqp_thread_t *next = queues[q][rr_index[q] % queue_sizes[q]];
            rr_index[q] = (rr_index[q] + 1) % queue_sizes[q];

            current_thread = next;
            swapcontext(&main_context, &current_thread->context);

            // If not finished, move to lower priority queue
            if (!next->finished && q < NUM_QUEUES - 1)
            {
                // Insert at queue q+1
                queues[q + 1][queue_sizes[q + 1]++] = next;
            }
        }
#undef NUM_QUEUES
        break;
    }

    default:
        /* Fallback: same as NQP_SP_TWOTHREADS. */
        current_index = 0;
        current_thread = thread_queue[current_index];
        swapcontext(&main_context, &current_thread->context);
        break;
    }
}
