#include <stdlib.h>
#include <assert.h>
#include <ucontext.h>
#include <signal.h>

#include "nqp_thread.h"
#include "nqp_thread_sched.h"

typedef struct thread_control_block
{
    ucontext_t context;
    void *stack;
    // Optionally, add thread ID, status, priority, etc.
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

    // Allocate a new stack for the thread -- Look back at the struct for the new_thread!
    new_thread->stack = malloc(SIGSTKSZ);
    if (new_thread->stack == NULL)
    {
        free(new_thread);
        return NULL;
    }

    // Now have to set the context for the new thread !
    // Initialize the thread's context.
    if (getcontext(&new_thread->context) == -1)
    {
        free(new_thread->stack);
        free(new_thread);
        return NULL;
    }

    // set up the stack
    new_thread->context.uc_stack.ss_sp = new_thread->stack;
    new_thread->context.uc_stack.ss_size = SIGSTKSZ;
    new_thread->context.uc_stack.ss_flags = 0;
    // You can set uc_link to a context to return to when this thread finishes.
    // For now, we'll set it to NULL.
    new_thread->context.uc_link = NULL;

    // Configure the context to begin execution in the task function.
    // makecontext expects a function that takes no arguments, so we cast.
    // The '1' indicates that one argument will be passed.
    makecontext(&new_thread->context, (void (*)(void))task, 1, arg);

    return new_thread;
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

    if (thread != NULL)
    {
    }

    return -1;
}

// --------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------

// 4 Fucntions from nqp_thread_sched.h !
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
