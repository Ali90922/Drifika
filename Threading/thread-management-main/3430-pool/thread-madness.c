#include "3430-pool.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#define THREADS 2

long sum = 0;

void *work( void *args )
{
    uint32_t *amount = (uint32_t *) args;
    uint32_t i;

    for ( i = 0; i < *amount; i++ )
    {
        sum = sum + 1;
    }

    return NULL;
}

int main( void )
{
    thr_pool_t *pool = thr_pool_create( THREADS );

    uint32_t total_jobs = 1024;
    uint32_t job;
    printf( "About to submit %u jobs\n", total_jobs );
    for ( job = 0; job < total_jobs; job++ )
    {
        thr_pool_queue( pool, work, &total_jobs );
    }
    printf( "Finished queuing work, waiting.\n" );

    thr_pool_wait( pool );
    printf( "The total sum is %ld\n", sum );
    return EXIT_SUCCESS;
}
