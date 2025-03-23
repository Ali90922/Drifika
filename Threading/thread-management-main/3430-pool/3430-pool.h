#pragma once
#include <pthread.h>
#include <stdint.h>

typedef	struct thr_pool	thr_pool_t;

/*
 * Create a thread pool.
 * 
 * args:
 *  threads: the number of threads to keep running.
 * return: an instance of the thread pool or NULL on error.
 */
thr_pool_t *thr_pool_create(uint16_t threads);

/*
 * Enqueue a work request to the thread pool job queue.  If there are idle
 * worker threads, awaken one to perform the job.  Else just return after
 * adding the job to the queue.
 *
 * args:
 *  pool: The pool in which to queue the task.
 *  func: the function the thread should run.
 * return: 0 on success, -1 on error.
 *
 */
int	thr_pool_queue(thr_pool_t *pool,
			void *(*func)(void *), void *arg);

/*
 * Wait for all queued jobs to complete.
 *
 * This function blocks while there are threads in the pool still running tasks.
 *
 * args:
 *  pool: the pool to wait on all jobs.
 */
void thr_pool_wait(thr_pool_t *pool);

