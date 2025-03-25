#include <stdatomic.h>
#include "nqp_thread_locks.h"
#include <stdlib.h>

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


// Define the internal structure for your mutex.
struct NQP_THREAD_MUTEX_T {
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
    if (!mutex) {
        return NULL; // Allocation failed.
    }
    
    // Initialize the atomic flag to the unlocked state.
    atomic_flag_clear(&mutex->flag);
    
    return mutex;
}


int nqp_thread_mutex_lock(nqp_mutex_t *mutex)
{
    (void)mutex;

    return -1;
}

int nqp_thread_mutex_trylock(nqp_mutex_t *mutex)
{
    (void)mutex;

    return -1;
}

int nqp_thread_mutex_unlock(nqp_mutex_t *mutex)
{
    (void)mutex;

    return -1;
}

int nqp_thread_mutex_destroy(nqp_mutex_t *mutex)
{
    (void)mutex;

    return -1;
}
