#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "nqp_thread_locks.h"  // Your custom lock implementation

// Global shared variable
int counter = 0;

// Custom mutex
nqp_mutex_t *counter_lock;

/**
 * Function executed by each thread to increment 'counter' 1,000,000 times.
 * Uses your custom nqp mutex to prevent race conditions.
 */
void *thread_counter(void *args)
{
    (void)args; // Avoid unused parameter warning

    // Lock your custom spinlock
    nqp_thread_mutex_lock(counter_lock);

    for (int i = 0; i < 1000000; i++)
    {
        counter++;
    }

    // Unlock your custom spinlock
    nqp_thread_mutex_unlock(counter_lock);

    // Two options - can also have the locks within the loop -- just above when we increment counter and below the increment

    return NULL;
}

int main(void)
{
    pthread_t t1, t2;

    // Initialize your custom mutex
    counter_lock = nqp_thread_mutex_init();
    if (!counter_lock)
    {
        fprintf(stderr, "Failed to initialize custom mutex\n");
        return EXIT_FAILURE;
    }

    // Create threads
    pthread_create(&t1, NULL, thread_counter, NULL);
    pthread_create(&t2, NULL, thread_counter, NULL);

    // Wait for both threads to finish
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    // Print the final result
    printf("Value of counter is %d\n", counter);

    // Destroy your custom mutex
    nqp_thread_mutex_destroy(counter_lock);

    return EXIT_SUCCESS;
}

