#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "job-queue.h"

struct JOB_QUEUE
{
    struct JOB_NODE *head;
    uint32_t size;
};

struct JOB_NODE
{
    job_t *job;
    struct JOB_NODE *next;
};

void check_queue( const job_queue *q )
{
    uint32_t count = 0;
    struct JOB_NODE *curr;

    assert( q != NULL );

#ifndef NDEBUG
    curr = q->head;

    while( curr != NULL )
    {
        count++;
        curr = curr->next;
    }

    assert( count == q->size );
#endif
}

job_queue *make_queue( void )
{
    job_queue *q = malloc(sizeof( job_queue ));

    if ( q )
    {
        q->size = 0;
        q->head = NULL;
        check_queue ( q );
    }

    return q;
}

int queue_size( const job_queue *q )
{
    check_queue( q );
    int size = -1;

    if ( q != NULL )
    {
        size = q->size;
    }

    return size;
}

void enqueue( job_queue *q, const job_t *job )
{
    check_queue( q );
    assert( job != NULL );
    struct JOB_NODE *n = NULL;

    if ( job != NULL && q != NULL )
    {
        n = malloc(sizeof(struct JOB_NODE));
        n->job = malloc(sizeof( struct JOB ));
        memcpy( n->job, job, sizeof( struct JOB ));
        n->next = q->head;

        q->head = n;
        q->size++;
    }
    check_queue( q );
}

job_t *dequeue( job_queue *q )
{
    check_queue( q );
    struct JOB_NODE *n = NULL, *prev = NULL;
    job_t *dequeued = NULL;

    if ( q != NULL && q->size > 0)
    {
        n = q->head;
        while ( n->next != NULL )
        {
            prev = n;
            n = n->next;
        }

        dequeued = n->job;

        if ( prev != NULL )
        {            
            prev->next = NULL;
        }
        else
        {
            // if prev is NULL, we never entered the loop
            // so the item we dequeued is head.
            q->head = NULL;
        }

        free(n);

        q->size--;
    }

    check_queue( q );

    return dequeued;
}

