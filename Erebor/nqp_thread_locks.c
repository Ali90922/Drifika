#include <stdatomic.h>
#include "nqp_thread_locks.h"

#include "nqp_thread_sched.h"
// Need the above file for the NQP Yeilf function
#include <stdlib.h>

// After SPIN_THRESHOLD iterations, the thread will yield its time slice.
#define SPIN_THRESHOLD 1000

// Assignment Instructions :
/*

In this part you will implement your own locks as defined in the interface in nqp_thread_locks.h.

Lock implementations require the use of atomic instructions and/or data types.
You can and should use the atomic instructions and/or data types
that are defined in stdatomic.h to implement your locks.

You should implement your locks as spin locks using the atomic primitives in stdatomic.h.

You should evaluate your implementation of locks for correctness (e.g., enforcing mutual
exclusion of two concurrent threads of execution) by using them in a program with pthreads.
 You are permitted to copy one of the concurrent code samples from class (e.g., the bad adding
  code) and modify the code to use your lock interface.

As you continue your implementation of concurrency: when a lock cannot be acquired by a thread
 it should yield (i.e., call nqp_yield as defined in nqp_thread_sched.h). Your implementation
  may still spin prior to yielding, but spinning should not be “infinite” (your thread must
  eventually give up control because no other thread can release the lock until your thread
  has given up control).
*/

// Define the internal structure for my mutex.
struct NQP_THREAD_MUTEX_T
{
    atomic_flag flag;
};

/**
 * Initialize an nqp_mutex_t.
 *
 * Return: An initialized lock or NULL on error.
 */
nqp_mutex_t *nqp_thread_mutex_init(void)
{
    // Allocate memory for the mutex.
    nqp_mutex_t *mutex = malloc(sizeof(struct NQP_THREAD_MUTEX_T));
    if (!mutex)
    {
        return NULL; // Allocation failed.
    }

    // Initialize the atomic flag to the unlocked state.
    atomic_flag_clear(&mutex->flag);

    return mutex;
}

/**
 * Lock the passed mutex. Will either immediately return and the calling thread
 * will have acquired the mutex, or will block until the mutex is available to
 * be acquired by the calling thread.
 *
 * Args:
 *  mutex: must not be NULL, must have previously been initialized.
 * Return: 0 on success, -1 on error.
 */

int nqp_thread_mutex_lock(nqp_mutex_t *mutex)
{
    if (!mutex)
    {
        return -1; // Error: NULL pointer provided.
    }

    // Then check the state of the lock
    int spin_count = 0;
    // Attempt to acquire the lock in a loop.
    // atomic_flag_test_and_set_explicit sets the flag and returns its previous value.
    // If the flag was clear, it returns false and the lock is acquired.
    while (atomic_flag_test_and_set_explicit(&mutex->flag, memory_order_acquire))
    {
        // only enters this while loop condition of the above condition evalutes to true --
        // meaning that the lock was not available to acquire

        // If we've spun for a while, yield control to allow other threads to run.
        if (++spin_count >= SPIN_THRESHOLD)
        {
            nqp_yield();
            spin_count = 0;
        }
    }
    return 0;
}

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
int nqp_thread_mutex_trylock(nqp_mutex_t *mutex)
{
    if (!mutex)
    {
        return -1; // Error: NULL pointer provided.
    }

    // Try to atomically set the flag.
    // If the flag was clear, then the lock is acquired.
    if (!atomic_flag_test_and_set_explicit(&mutex->flag, memory_order_acquire))
    {
        return 0; // Successfully acquired the lock.
    }

    return 1; // Lock is already held by another thread.
}

/**
 * Release a previously acquired mutex.
 *
 * Args:
 *  mutex: must not be NULL, must have been previously initialized.
 * Return: 0 on success, -1 on error.
 */

int nqp_thread_mutex_unlock(nqp_mutex_t *mutex)
{
    if (!mutex)
    {
        return -1; // Error: NULL pointer provided.
    }

    // Release the lock by clearing the atomic flag.
    atomic_flag_clear_explicit(&mutex->flag, memory_order_release);
    return 0;
}

/**
 * Destroy a mutex. The mutex should not be re-used after calling this function.
 *
 * Args:
 *  mutex: must not be NULL, must have been previously initialized.
 * Returns: 0 on success, -1 on error.
 */

int nqp_thread_mutex_destroy(nqp_mutex_t *mutex)
{
    if (!mutex)
    {
        return -1; // Error: NULL pointer provided.
    }

    free(mutex);
    return 0;
}
