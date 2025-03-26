#include <stdlib.h>
#include <assert.h>
#include <ucontext.h>

#include "nqp_thread.h"
#include "nqp_thread_sched.h"

typedef struct thread_control_block
{
    ucontext_t context;
    void *stack;
    // Optionally, add thread ID, status, priority, etc.
} tcb_t;

static nqp_scheduling_policy system_policy = NQP_SP_TWOTHREADS;

nqp_thread_t *nqp_thread_create(void (*task)(void *), void *arg)
{
    assert(task != NULL);
    (void)arg;

    if (task != NULL)
    {
    }

    return NULL;
}

int nqp_thread_join(nqp_thread_t *thread)
{
    assert(thread != NULL);

    if (thread != NULL)
    {
    }

    return -1;
}

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
