#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <ucontext.h>
#include <signal.h>

#include "nqp_thread.h"
#include "nqp_thread_sched.h"

#define MAX_THREADS 50

// Global scheduler queue: holds pointers to all NQP threads that are runnable.
static nqp_thread_t *thread_queue[MAX_THREADS];
static int num_threads = 0;   // How many threads have been added.
static int current_index = 0; // Index of the currently running thread.
static nqp_thread_t *current_thread = NULL;

static nqp_scheduling_policy system_policy = NQP_SP_TWOTHREADS;

typedef struct nqp_thread_t
{
    ucontext_t context;
    void *stack;
    int finished;
    int id; // Unique identifier (optional).
} nqp_thread_t;

static void thread_wrapper(void (*task)(void *), void *arg, nqp_thread_t *thread);
static void scheduler_add_thread(nqp_thread_t *thread);

// ------------- THREAD CREATION & WRAPPER -------------
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
    new_thread->context.uc_stack.ss_sp = new_thread->stack;
    new_thread->context.uc_stack.ss_size = SIGSTKSZ;
    new_thread->context.uc_stack.ss_flags = 0;
    new_thread->context.uc_link = NULL;

    // Wrap the user function in thread_wrapper, which marks finished & calls nqp_exit.
    makecontext(&new_thread->context, (void (*)(void))thread_wrapper, 3, task, arg, new_thread);

    // Add the new thread to the scheduler queue
    scheduler_add_thread(new_thread);

    return new_thread;
}

static void thread_wrapper(void (*task)(void *), void *arg, nqp_thread_t *thread)
{
    // Execute the user-provided task.
    task(arg);
    // Mark as finished and exit.
    thread->finished = 1;
    nqp_exit();
}

static void scheduler_add_thread(nqp_thread_t *thread)
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

// ------------- JOIN / WAITING -------------
int nqp_thread_join(nqp_thread_t *thread)
{
    assert(thread != NULL);
    // Simple busy-wait with yielding.
    while (!thread->finished)
    {
        nqp_yield();
    }
    return 0;
}

// ------------- SCHEDULING API -------------
int nqp_sched_init(const nqp_scheduling_policy policy, const nqp_sp_settings *settings)
{
    (void)settings;
    // Simple bounds check
    if (policy < NQP_SP_TWOTHREADS || policy >= NQP_SP_POLICIES)
        return -1;

    system_policy = policy;
    return 0;
}

/**
 * nqp_yield:
 *   Voluntarily give up control.
 *   In this refactor, if the scheduling is done inside nqp_sched_start() loops
 *   (FIFO, RR, MLFQ), then yield does very little, because “who runs next” is chosen
 *   by the big loop in nqp_sched_start.
 *   If the scheduling is truly yield-based (like your two-thread policy),
 *   we do the real scheduling in yield.
 */
void nqp_yield(void)
{
    // If we're not running inside an NQP thread, do nothing.
    if (current_thread == NULL)
        return;

    switch (system_policy)
    {
    case NQP_SP_FIFO:
        // FIFO policy in your code is driven by a big while-loop in nqp_sched_start.
        // So yield is effectively a no-op.
        return;

    case NQP_SP_RR:
        // Round-robin in your code is also driven by a while loop in nqp_sched_start.
        // So do nothing here—any time-slice logic is in nqp_sched_start.
        return;

    case NQP_SP_MLFQ:
        // MLFQ in your code is likewise in nqp_sched_start’s loop.
        // So yield is no-op here, or do any minimal “tracking” if needed.
        return;

    case NQP_SP_TWOTHREADS:
    {
        // The “two-threads only” approach never returns to nqp_sched_start,
        // so we *must* do the scheduling right here.
        nqp_thread_t *prev = current_thread;

        // Flip to the next index (just 2 threads assumed).
        int next_index = (current_index + 1) % num_threads;
        nqp_thread_t *next = thread_queue[next_index];

        // If the other thread is finished, do nothing.
        // Or if there's only 1 thread left, etc.
        if (!next->finished && next != prev)
        {
            current_index = next_index;
            current_thread = next;
            swapcontext(&prev->context, &next->context);
        }
        return;
    }

    default:
        // Fallback: do nothing
        return;
    }
}

/**
 * nqp_exit:
 *   The current thread calls nqp_exit() to mark itself finished and let someone else run.
 */
void nqp_exit(void)
{
    if (current_thread)
        current_thread->finished = 1;

    // Call yield so that we switch away if we're in two-thread mode.
    // If in FIFO/RR/MLFQ, nqp_sched_start’s big loop will eventually skip the finished thread anyway.
    nqp_yield();

    // If we ever come back here, just exit the entire process to avoid confusion.
    exit(0);
}

/**
 * nqp_sched_start:
 *   Start scheduling tasks. Depending on the policy:
 *    - NQP_SP_TWOTHREADS: never returns (swaps once, then yields does everything).
 *    - NQP_SP_FIFO, NQP_SP_RR, NQP_SP_MLFQ: runs the big scheduling loop you originally wrote.
 */
void nqp_sched_start(void)
{
    if (num_threads == 0)
        return;

    ucontext_t main_context; // So we can swap out of "main".

    if (system_policy == NQP_SP_TWOTHREADS)
    {
        // Two-thread policy: just pick the first thread and swap to it.
        current_index = 0;
        current_thread = thread_queue[current_index];
        swapcontext(&main_context, &current_thread->context);
        // Because yield will keep flipping back and forth, we never return.
    }
    else if (system_policy == NQP_SP_FIFO)
    {
        // The “big loop” approach: keep running the first unfinished thread
        // until it finishes, then run the next, etc.
        while (1)
        {
            nqp_thread_t *next = NULL;
            // Find the first worker that’s not finished
            for (int i = 0; i < num_threads; i++)
            {
                if (!thread_queue[i]->finished)
                {
                    next = thread_queue[i];
                    break;
                }
            }
            if (!next)
                break; // all done

            current_thread = next;
            swapcontext(&main_context, &current_thread->context);
            // Once we get back here, if 'next' is not finished,
            // we continue the loop (i.e., keep picking it again).
        }
    }
    else if (system_policy == NQP_SP_RR)
    {
        // Round-robin big loop.
        current_index = 0;
        current_thread = thread_queue[current_index];

        while (1)
        {
            // Check if all threads are finished:
            int unfinished = 0;
            for (int i = 0; i < num_threads; i++)
            {
                if (!thread_queue[i]->finished)
                {
                    unfinished = 1;
                    break;
                }
            }
            if (!unfinished)
                break; // done

            // Move to next thread, skipping finished
            current_index = (current_index + 1) % num_threads;
            while (thread_queue[current_index]->finished)
                current_index = (current_index + 1) % num_threads;

            current_thread = thread_queue[current_index];
            swapcontext(&main_context, &current_thread->context);
        }
    }
    else if (system_policy == NQP_SP_MLFQ)
    {
// Simple MLFQ with 3 queues, round-robin in each.
#define NUM_QUEUES 3
        nqp_thread_t *queues[NUM_QUEUES][MAX_THREADS] = {0};
        int queue_sizes[NUM_QUEUES] = {0};
        int rr_indices[NUM_QUEUES] = {0};

        // Initialize all threads into the top queue (queue 0).
        for (int i = 0; i < num_threads; i++)
        {
            queues[0][queue_sizes[0]++] = thread_queue[i];
        }

        while (1)
        {
            // Check if all are finished.
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

            // Find the highest non-empty queue
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
                break; // all done

            // Round-robin pick in queue q
            nqp_thread_t *next = queues[q][rr_indices[q] % queue_sizes[q]];
            rr_indices[q] = (rr_indices[q] + 1) % queue_sizes[q];

            current_thread = next;
            swapcontext(&main_context, &current_thread->context);

            // If it's not finished, we demote it to the next queue
            // (you might do more “time quantum” accounting here).
            if (!next->finished && q < NUM_QUEUES - 1)
            {
                // Insert it into queue q+1
                queues[q + 1][queue_sizes[q + 1]++] = next;
            }
        }
#undef NUM_QUEUES
    }
    else
    {
        // Default fallback -> treat like NQP_SP_TWOTHREADS
        current_index = 0;
        current_thread = thread_queue[current_index];
        swapcontext(&main_context, &current_thread->context);
    }
}
