#pragma once

typedef struct NQP_THREAD_MUTEX_T nqp_mutex_t;

/**
 * Initialize an nqp_mutex_t. 
 *
 * Return: An initialized lock or NULL on error.
 */
nqp_mutex_t *nqp_thread_mutex_init( void );

/**
 * Lock the passed mutex. Will either immediately return and the calling thread
 * will have acquired the mutex, or will block until the mutex is available to
 * be acquired by the calling thread.
 *
 * Args:
 *  mutex: must not be NULL, must have previously been initialized.
 * Return: 0 on success, -1 on error.
 */
int nqp_thread_mutex_lock( nqp_mutex_t *mutex );

/**
 * Try to lock the passed mutex. This function will always immediately return 
 * (this function will never block the calling thread). The value returned
 * indicates whether or not the mutex was acquired by the caller. Caller is
 * responsible for checking whether or not it has successfully acquired the
 * mutex before entering a critical section.
 *
 * Args:
 *  mutex: must not be NULL, must have been previously initialized.
 * Return:
 *  * 0 on success (the lock has been acquired).
 *  * A positive, non-zero value on failure to acquire the mutex.
 *  * -1 on error.
 */
int nqp_thread_mutex_trylock( nqp_mutex_t *mutex );

/**
 * Release a previously acquired mutex.
 *
 * Args:
 *  mutex: must not be NULL, must have been previously initialized.
 * Return: 0 on success, -1 on error.
 */
int nqp_thread_mutex_unlock( nqp_mutex_t *mutex );

/**
 * Destroy a mutex. The mutex should not be re-used after calling this function.
 *
 * Args:
 *  mutex: must not be NULL, must have been previously initialized.
 * Returns: 0 on success, -1 on error.
 */
int nqp_thread_mutex_destroy( nqp_mutex_t *mutex );
