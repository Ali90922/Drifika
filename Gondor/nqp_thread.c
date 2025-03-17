#include <stdlib.h>
#include <assert.h>
#include "nqp_thread.h"

nqp_thread_t *nqp_thread_create( void (*task)(void *), void *arg )
{
    assert( task != NULL );
    (void) arg;

    if ( task != NULL )
    {

    }

    return NULL;
}

int nqp_thread_join( nqp_thread_t *thread )
{
    assert( thread != NULL );

    if ( thread != NULL )
    {

    }
    
    return -1;
}
