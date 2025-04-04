#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <ucontext.h>
#include <signal.h>
#include <time.h>

#include "nqp_thread.h"
#include "nqp_thread_sched.h"

#define MAX_THREADS 50

// Global thread queue:
static nqp_thread_t *thread_queue[MAX_THREADS];
static int num_threads = 0;   // Count of threads in the scheduler.
static int current_index = 0; // Index of the currently running thread.
static nqp_thread_t *current_thread = NULL;

// Global scheduler context for MLFQ mode.
static ucontext_t scheduler_context;

// Thread control block.
typedef struct nqp_thread_t
{
    ucontext_t context;
    void *stack;
    int finished; // 0: running, 1: finished.
    int id;       // Unique thread identifier.
    // Optionally, add fields for MLFQ (e.g., current queue, runtime, etc.).
} nqp_thread_t;

static nqp_scheduling_policy system_policy = NQP_SP_TWOTHREADS;

/* nqp_thread_create: creates and initializes a thread */
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
    // (Optional) assign a unique id here.

    new_thread->context.uc_stack.ss_sp = new_thread->stack;
    new_thread->context.uc_stack.ss_size = SIGSTKSZ;
    new_thread->context.uc_stack.ss_flags = 0;
    new_thread->context.uc_link = NULL;

    // Setup the thread to run thread_wrapper.
    makecontext(&new_thread->context, (void (*)(void))thread_wrapper, 3, task, arg, new_thread);

    scheduler_add_thread(new_thread);
    return new_thread;
}

/* thread_wrapper: executes the task then marks the thread finished */
void thread_wrapper(void (*task)(void *), void *arg, nqp_thread_t *thread)
{
    task(arg);
    thread->finished = 1;
    nqp_exit();
}

/* scheduler_add_thread: adds a thread to the global thread queue */
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

/* nqp_thread_join: busy-wait until thread finishes */
int nqp_thread_join(nqp_thread_t *thread)
{
    assert(thread != NULL);
    while (!thread->finished)
    {
        nqp_yield();
    }
    return 0;
}

/* nqp_sched_init: sets the scheduling policy */
int nqp_sched_init(const nqp_scheduling_policy policy,
                   const nqp_sp_settings *settings)
{
    (void)settings;
    int ret = -1;
    assert(policy >= NQP_SP_TWOTHREADS && policy < NQP_SP_POLICIES);
    if (policy >= NQP_SP_TWOTHREADS && policy < NQP_SP_POLICIES)
    {
        system_policy = policy;
        ret = 0;
    }
    return ret;
}

/* nqp_yield: yields control to the scheduler.
 * For MLFQ, swap back to the scheduler context.
 */
void nqp_yield(void)
{
    if (!current_thread)
        return;

    if (system_policy == NQP_SP_MLFQ)
    {
        /* In MLFQ mode, yield back to the scheduler */
        swapcontext(&current_thread->context, &scheduler_context);
        return;
    }

    if (system_policy == NQP_SP_TWOTHREADS || system_policy == NQP_SP_RR)
    {
        nqp_thread_t *prev = current_thread;
        int next_index = (current_index + 1) % num_threads;
        nqp_thread_t *next = thread_queue[next_index];

        int iterations = 0;
        while (next->finished && iterations < num_threads)
        {
            next_index = (next_index + 1) % num_threads;
            next = thread_queue[next_index];
            iterations++;
        }
        if (next == prev || next->finished)
            return;
        current_index = next_index;
        current_thread = next;
        swapcontext(&prev->context, &next->context);
        return;
    }

    if (system_policy == NQP_SP_FIFO)
    {
        if (!current_thread->finished)
            return;
        nqp_thread_t *prev = current_thread;
        nqp_thread_t *next = NULL;
        int next_i = -1;
        for (int i = 0; i < num_threads; i++)
        {
            if (!thread_queue[i]->finished)
            {
                next = thread_queue[i];
                next_i = i;
                break;
            }
        }
        if (!next)
            return;
        current_thread = next;
        current_index = next_i;
        swapcontext(&prev->context, &next->context);
        return;
    }
}

/* nqp_exit: marks the current thread as finished and yields */
void nqp_exit(void)
{
    if (current_thread != NULL)
        current_thread->finished = 1;
    nqp_yield();
    exit(0);
}

/* nqp_sched_start: starts the scheduling of threads.
 * For non-MLFQ policies, simply start the first thread.
 * For MLFQ, run a scheduling loop with time-slice measurement and boosting.
 */
void nqp_sched_start(void)
{
    if (num_threads == 0)
        return;

    if (system_policy != NQP_SP_MLFQ)
    {
        ucontext_t main_context;
        current_index = 0;
        current_thread = thread_queue[current_index];
        swapcontext(&main_context, &current_thread->context);
    }
    else
    {
        // ---- MLFQ Scheduling Implementation ----
#define NUM_QUEUES 3
        // Persistent arrays for the three MLFQ queues.
        static nqp_thread_t *mlfq_queues[NUM_QUEUES][MAX_THREADS] = {{0}};
        static int mlfq_queue_sizes[NUM_QUEUES] = {0};
        static int rr_indices[NUM_QUEUES] = {0};

        // Time-slice and boost parameters.
        const long time_slice_us = 125000;      // 125,000 µs = 0.125 sec.
        const long boost_interval_us = 2000000; // 2,000,000 µs = 2 sec.

        // Record the last boost time.
        struct timespec last_boost_time;
        clock_gettime(CLOCK_REALTIME, &last_boost_time);

        // Initially, place all threads in the highest-priority queue (queue 0).
        for (int i = 0; i < num_threads; i++)
        {
            mlfq_queues[0][mlfq_queue_sizes[0]++] = thread_queue[i];
        }

        // Initialize the scheduler context.
        getcontext(&scheduler_context);

        while (1)
        {
            // Check if all threads have finished.
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

            // Boost: if boost interval has elapsed, move all unfinished threads to queue 0.
            struct timespec now;
            clock_gettime(CLOCK_REALTIME, &now);
            long elapsed_us = (now.tv_sec - last_boost_time.tv_sec) * 1000000L +
                              (now.tv_nsec - last_boost_time.tv_nsec) / 1000;
            if (elapsed_us >= boost_interval_us)
            {
                for (int q = 1; q < NUM_QUEUES; q++)
                {
                    for (int j = 0; j < mlfq_queue_sizes[q]; j++)
                    {
                        nqp_thread_t *t = mlfq_queues[q][j];
                        if (!t->finished)
                        {
                            mlfq_queues[0][mlfq_queue_sizes[0]++] = t;
                        }
                    }
                    mlfq_queue_sizes[q] = 0;
                    rr_indices[q] = 0;
                }
                last_boost_time = now;
            }

            // Find the highest priority queue with runnable threads.
            int queue_level = -1;
            for (int q = 0; q < NUM_QUEUES; q++)
            {
                if (mlfq_queue_sizes[q] > 0)
                {
                    queue_level = q;
                    break;
                }
            }
            if (queue_level == -1)
                break; // No runnable threads found.

            // Select the next thread using round-robin from the chosen queue.
            int idx = rr_indices[queue_level] % mlfq_queue_sizes[queue_level];
            nqp_thread_t *next = mlfq_queues[queue_level][idx];
            rr_indices[queue_level] = (rr_indices[queue_level] + 1) % mlfq_queue_sizes[queue_level];

            // If the selected thread is finished, remove it from the queue.
            if (next->finished)
            {
                mlfq_queues[queue_level][idx] = mlfq_queues[queue_level][mlfq_queue_sizes[queue_level] - 1];
                mlfq_queue_sizes[queue_level]--;
                continue;
            }

            // Record the start time for this time slice.
            struct timespec start_time;
            clock_gettime(CLOCK_REALTIME, &start_time);

            current_thread = next;
            /* Switch from scheduler context to the thread.
             * When the thread calls nqp_yield, it will swap back into scheduler_context.
             */
            swapcontext(&scheduler_context, &current_thread->context);

            // When the thread yields, record the end time.
            struct timespec end_time;
            clock_gettime(CLOCK_REALTIME, &end_time);
            long slice_us = (end_time.tv_sec - start_time.tv_sec) * 1000000L +
                            (end_time.tv_nsec - start_time.tv_nsec) / 1000;

            // If the thread used up its time slice, demote it to a lower queue (if not already at the lowest level).
            if (slice_us >= time_slice_us && queue_level < NUM_QUEUES - 1)
            {
                // Remove from current queue by replacing it with the last element.
                mlfq_queues[queue_level][idx] = mlfq_queues[queue_level][mlfq_queue_sizes[queue_level] - 1];
                mlfq_queue_sizes[queue_level]--;
                // Add the thread to the next lower queue.
                mlfq_queues[queue_level + 1][mlfq_queue_sizes[queue_level + 1]++] = next;
            }
            // Otherwise, leave the thread in its current queue.
        }
#undef NUM_QUEUES
    }
}
