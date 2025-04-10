#include <stdio.h>
#include <stdlib.h>
#include "job-queue.h"

void *test_function(void *arg)
{
    (void) arg;
    return NULL;
}

int main(void)
{
    job_queue *q = make_queue();
    job_t j = {
        .job_func = test_function,
        .job_arg = NULL
    };

    if ( queue_size( q ) != 0 )
    {
        fprintf(stderr, "Queue should be empty, but reports size != 0.\n");
    }

    enqueue( q, &j );

    if ( queue_size( q ) != 1 )
    {
        fprintf(stderr, "Queue should have 1 item, but reports size != 1.\n");
    }

    j.job_arg = q;
    
    enqueue( q, &j );

    if ( queue_size( q ) != 2 )
    {
        fprintf(stderr, "Queue should have 2 items, but reports size != 2.\n");
    }


    job_t *dequeued = dequeue( q );

    if ( dequeued->job_arg != NULL ) 
    {
        fprintf(stderr, "Got the wrong job; first queued job was NULL.\n");
    }
    if ( queue_size( q ) != 1 )
    {
        fprintf(stderr, "Queue should have 1 item, but reports size != 1.\n");
    }

   
    dequeued = dequeue( q );

    if ( dequeued->job_arg != q )
    {
        fprintf(stderr, "Got the wrong job; second queued job was q.\n");
    }
    if ( queue_size( q ) != 0 )
    {
        fprintf(stderr, "Queue should have no items, but reports size != 0.\n");
    }


    return EXIT_SUCCESS;
}
