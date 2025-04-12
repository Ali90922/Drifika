#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#include "3430-pool.h"
#include "job-queue.h"

struct thr_pool {
  pthread_mutex_t pool_mutex; 
  pthread_cond_t pool_workcv; 
  job_queue *jobs;
  int pool_nthreads;
  bool pool_wait;
};

static void *worker_thread(void *arg) {
  thr_pool_t *pool = (thr_pool_t *)arg;
  job_t *job;
  void *(*func)(void *);

  (void) arg;
  (void) job;
  (void) func;

  assert( pool != NULL );
  // job of the thread:
  // 1. Check to see if there is work (something
  // in the queue for us to work on).
  //    * If work: Then do the work
  //    * If not work: go to sleep/suspend/block
  //      until someone tells you there is work.
  // 2. If you're being told to shut down,
  //    then shut down.
  while ( pool != NULL && !pool->pool_wait )
  {
    // I need to acquire the lock before I do anything with the condition
    // variable and also before I even ask how big my queue is.
    pthread_mutex_lock( &pool->pool_mutex );

    while ( queue_size( pool->jobs ) == 0 && !pool->pool_wait )
    {
        // if there is no work to do, suspend yourself and release the lock
        pthread_cond_wait( &pool->pool_workcv, &pool->pool_mutex );
    }

    if ( queue_size( pool->jobs ) > 0 && !pool->pool_wait )
    {
        // I hold the lock, so I can make changes to the queue.
        job = dequeue( pool->jobs );

        func = job->job_func;
        arg  = job->job_arg;

        // once I hold the job, I can release the lock
        pthread_mutex_unlock( &pool->pool_mutex );

        // do the work
        func( arg );
    }
    else if ( pool->pool_wait )
    {
        // if we have been told to shut down, then release the lock
        pthread_mutex_unlock( &pool->pool_mutex );
    }
  }

  // exiting, tell everyone else:
  pthread_mutex_lock( &pool->pool_mutex );
  pool->pool_nthreads--;
  pthread_mutex_unlock( &pool->pool_mutex );

  return NULL;
}

thr_pool_t *thr_pool_create(uint16_t threads)
{
  thr_pool_t *pool;
  pthread_t *thread;

  assert( threads > 0 );

  pool = calloc(1, sizeof(thr_pool_t));

  pthread_mutex_init(&pool->pool_mutex, NULL);
  pthread_cond_init(&pool->pool_workcv, NULL);

  pool->jobs = make_queue( );
  pool->pool_wait = false;
  pool->pool_nthreads = threads;

  // start all the threads; they shoud immediately go
  // idle because there isn't any work yet.
  while( threads > 0 ) 
  {
    thread = calloc(1, sizeof(pthread_t));
    pthread_create(thread, NULL, worker_thread, pool);
    threads--;
  }

  return pool;
}

int thr_pool_queue(thr_pool_t *pool, void *(*func)(void *), void *arg) {
  job_t *job;
  int status = -1;

  (void) job;
  (void) arg;

  assert( pool != NULL );
  assert( func != NULL );

  if ( pool != NULL && func != NULL )
  {
    job = calloc(1, sizeof(job_t));

    if (job != NULL )
    {
       job->job_func = func;
       job->job_arg  = arg;

       // I want to enqueue the job, so make sure nobody else is
       // modifying the queue while I am modifying the queue, or
       // trying to read the queue while I am modifying the queue.
       pthread_mutex_lock( &pool->pool_mutex );
       enqueue( pool->jobs, job );
       // signal to waiting threads that there is work for them to do
       pthread_cond_signal( &pool->pool_workcv ); // Wakes up 1 thread to do the work
       pthread_mutex_unlock( &pool->pool_mutex );   // Release the lock so the thread waking up from the wait call is able to acquire the lock
       status = 0;
    }
  }

  return status;
}

void thr_pool_wait(thr_pool_t *pool)
{
    assert( pool != NULL );

    if ( pool != NULL )
    {
        pool->pool_wait = true;

        while( pool->pool_nthreads > 0 )
        {
            pthread_mutex_lock( &pool->pool_mutex );
            pthread_cond_signal( &pool->pool_workcv );
            pthread_mutex_unlock( &pool->pool_mutex );
        }
    }
}
