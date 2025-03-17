#include <assert.h>
#include <stdlib.h>

#include "nqp_thread_sched.h"

static nqp_scheduling_policy system_policy = NQP_SP_TWOTHREADS;

int nqp_sched_init( const nqp_scheduling_policy policy,
                    const nqp_sp_settings *settings )
{
    int ret = -1;
    (void) settings;

    assert( policy >= NQP_SP_TWOTHREADS && policy < NQP_SP_POLICIES );

    if ( policy >= NQP_SP_TWOTHREADS && policy < NQP_SP_POLICIES )
    {
        system_policy = policy;
        ret = 0;
    }
    return ret;
}

void nqp_yield( void )
{
    // schedule another (maybe different) task.
}

void nqp_exit( void )
{
    // remove the currently executing thread from the system.
}

int nqp_thread_time_left( const nqp_thread_t *thread )
{
    assert( thread != NULL );

    if ( thread != NULL )
    {
        // ask the thread how much time it has left.
    }

    return -1;
}
