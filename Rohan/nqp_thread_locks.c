#include <stdatomic.h>
#include "nqp_thread_locks.h"

nqp_mutex_t *nqp_thread_mutex_init( void )
{
    return NULL;
}

int nqp_thread_mutex_lock( nqp_mutex_t *mutex )
{
    (void) mutex;

    return -1;
}

int nqp_thread_mutex_trylock( nqp_mutex_t *mutex )
{
    (void) mutex;

    return -1;
}

int nqp_thread_mutex_unlock( nqp_mutex_t *mutex )
{
    (void) mutex;

    return -1;
}

int nqp_thread_mutex_destroy( nqp_mutex_t *mutex )
{
    (void) mutex;

    return -1;
}
