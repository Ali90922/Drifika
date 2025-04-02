#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <ucontext.h>
#include <signal.h>

#include "nqp_thread.h"
#include "nqp_thread_sched.h"

// For simplicity, assume we have a fixed-size thread queue.
#define MAX_THREADS 50

// --------------
// Thread Control
// --------------
typedef struct nqp_thread_t
{
    ucontext_t context;
    void *stack;
    int finished; // 0 means running, 1 means finished
    int id;       // Optional: unique identifier
} nqp_thread_t;

// Global scheduler queue: holds pointers to all NQP threads that are runnable
static nqp_thread_t *thread_queue[MAX_THREADS];
static int num_threads = 0;
static int current_index = 0;
static nqp_thread_t *current_thread = NULL;

// Global policy
static nqp_scheduling_policy system_policy = NQP_SP_TWOTHREADS;

// -------------------------------------------------------------------
// Internal Helper: Add thread to global queue
// -------------------------------------------------------------------
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

// -------------------------------------------------------------------
// Internal Helper: Thread wrapper
// -------------------------------------------------------------------
static void thread_wrapper(void (*task)(void *), void *arg, nqp_thread_t *thread)
{
    // Execute the user-provided function
    task(arg);

    // Mark the thread as finished
    thread->finished = 1;

    // Terminate this thread (and remove it from the scheduler)
    nqp_exit();

    // never returns
}

// -------------------------------------------------------------------
// nqp_thread_create
// -------------------------------------------------------------------
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
    new_thread->id = num_threads; // optional ID

    // Setup the stack
    new_thread->context.uc_stack.ss_sp = new_thread->stack;
    new_thread->context.uc_stack.ss_size = SIGSTKSZ;
    new_thread->context.uc_stack.ss_flags = 0;
    new_thread->context.uc_link = NULL;

    // Build a context that calls thread_wrapper(...)
    makecontext(&new_thread->context,
                (void (*)(void))thread_wrapper,
                3,
                task,
                arg,
                new_thread);

    // Add the new thread to our scheduler queue
    scheduler_add_thread(new_thread);

    return new_thread;
}

// -------------------------------------------------------------------
// nqp_thread_join
// -------------------------------------------------------------------
int nqp_thread_join(nqp_thread_t *thread)
{
    assert(thread != NULL);

    // Busy-wait with yielding until the thread is finished
    while (!thread->finished)
    {
        nqp_yield();
    }
    return 0; // successfully joined
}

// -------------------------------------------------------------------
// nqp_exit
// -------------------------------------------------------------------
void nqp_exit(void)
{
    // Mark the current thread as finished
    if (current_thread != NULL)
        current_thread->finished = 1;

    // Yield to allow the scheduler to pick another thread
    nqp_yield();

    // If we ever return here, no other thread was runnable
    exit(0);
}

// -------------------------------------------------------------------
// nqp_sched_init
// -------------------------------------------------------------------
int nqp_sched_init(const nqp_scheduling_policy policy,
                   const nqp_sp_settings *settings)
{
    (void)settings; // not used yet
    assert(policy >= NQP_SP_TWOTHREADS && policy < NQP_SP_POLICIES);

    if (policy >= NQP_SP_TWOTHREADS && policy < NQP_SP_POLICIES)
    {
        system_policy = policy;
        return 0;
    }
    return -1;
}

// -------------------------------------------------------------------
// nqp_yield
// -------------------------------------------------------------------
void nqp_yield(void)
{
    // If not running inside an NQP thread, do nothing.
    if (current_thread == NULL)
        return;

    // If FIFO, do nothing (the thread runs until it calls nqp_exit)
    if (system_policy == NQP_SP_FIFO)
        return;

    // For TWOTHREADS, RR, and MLFQ (as simple placeholder), do round-robin
    nqp_thread_t *prev = current_thread;
    int next_index = (current_index + 1) % num_threads;
    nqp_thread_t *next = thread_queue[next_index];

    // Skip finished threads
    int iterations = 0;
    while (next->finished && iterations < num_threads)
    {
        next_index = (next_index + 1) % num_threads;
        next = thread_queue[next_index];
        iterations++;
    }
    // If no available thread to run, just return
    if (next == prev)
        return;

    current_index = next_index;
    current_thread = next;
    swapcontext(&prev->context, &next->context);
}

// -------------------------------------------------------------------
// nqp_sched_start
// -------------------------------------------------------------------
void nqp_sched_start(void)
{
    if (num_threads == 0)
        return;

    ucontext_t main_context;

    if (system_policy == NQP_SP_TWOTHREADS)
    {
        // Swap once, never return.
        current_index = 0;
        current_thread = thread_queue[current_index];
        swapcontext(&main_context, &current_thread->context);
    }
    else if (system_policy == NQP_SP_FIFO)
    {
        // FIFO scheduling
        while (1)
        {
            // Find first unfinished thread
            nqp_thread_t *next = NULL;
            for (int i = 0; i < num_threads; i++)
            {
                if (!thread_queue[i]->finished)
                {
                    next = thread_queue[i];
                    break;
                }
            }
            // If none unfinished, break
            if (!next)
                break;

            current_thread = next;
            swapcontext(&main_context, &current_thread->context);
        }
    }
    else if (system_policy == NQP_SP_RR)
    {
        // Round-robin scheduling
        current_index = 0;
        current_thread = thread_queue[current_index];
        while (1)
        {
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
                break;

            current_index = (current_index + 1) % num_threads;
            while (thread_queue[current_index]->finished)
                current_index = (current_index + 1) % num_threads;

            current_thread = thread_queue[current_index];
            swapcontext(&main_context, &current_thread->context);
        }
    }
    else if (system_policy == NQP_SP_MLFQ)
    {
#define NUM_QUEUES 3
        // For MLFQ, we do a placeholder approach (similar to RR)
        // You can expand it with actual time slices, demotion, etc.
        nqp_thread_t *queues[NUM_QUEUES][MAX_THREADS] = {{0}};
        int queue_sizes[NUM_QUEUES] = {0};

        for (int i = 0; i < num_threads; i++)
        {
            queues[0][queue_sizes[0]++] = thread_queue[i];
        }
        int rr_indices[NUM_QUEUES] = {0};

        while (1)
        {
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

            // Find highest-priority non-empty queue
            int queue_level = -1;
            for (int i = 0; i < NUM_QUEUES; i++)
            {
                if (queue_sizes[i] > 0)
                {
                    queue_level = i;
                    break;
                }
            }
            if (queue_level == -1) // no unfinished
                break;

            // next thread from that queue
            nqp_thread_t *next = queues[queue_level][rr_indices[queue_level] % queue_sizes[queue_level]];
            rr_indices[queue_level] = (rr_indices[queue_level] + 1) % queue_sizes[queue_level];

            current_thread = next;
            swapcontext(&main_context, &current_thread->context);

            // demote if still not finished
            if (!next->finished && queue_level < NUM_QUEUES - 1)
            {
                // add to lower queue
                queues[queue_level + 1][queue_sizes[queue_level + 1]++] = next;
            }
        }
#undef NUM_QUEUES
    }
    else
    {
        // fallback = two-thread scheduling
        current_index = 0;
        current_thread = thread_queue[current_index];
        swapcontext(&main_context, &current_thread->context);
    }
}
