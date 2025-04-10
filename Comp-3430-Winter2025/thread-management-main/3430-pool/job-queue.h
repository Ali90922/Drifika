#pragma once

typedef struct JOB_QUEUE job_queue;

typedef struct JOB 
{
    void *(*job_func)(void *);
    void *job_arg;
} job_t;

job_queue *make_queue( void );
void enqueue( job_queue *q, const job_t *job );
job_t *dequeue( job_queue *q );
int queue_size( const job_queue *q );
