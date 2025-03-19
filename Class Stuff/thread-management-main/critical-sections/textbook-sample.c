#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

// Global shared variable 'counter' that both threads will modify
int counter = 0;

// Mutex (mutual exclusion) lock to synchronize access to 'counter' 
pthread_mutex_t counter_lock = PTHREAD_MUTEX_INITIALIZER;

/**
 * Function executed by each thread to increment 'counter' 1,000,000 times.
 * Uses a mutex to ensure only one thread modifies 'counter' at a time.
 */
void *thread_counter(void *args)
{
    (void) args; // Mark 'args' as unused to avoid compiler warnings

    // Lock the mutex before modifying 'counter' to prevent race conditions
    pthread_mutex_lock(&counter_lock);
    
    for (int i = 0; i < 1000000; i++)
    {
        // Since the entire loop is inside the mutex lock, only one thread 
        // can execute this loop at a time, ensuring correct updates to 'counter'.
        counter++;
    }

    // Unlock the mutex, allowing another thread to access 'counter'
    pthread_mutex_unlock(&counter_lock);

    return NULL; // Exit the thread
}

int main(void)
{
    pthread_t t1, t2; // Thread identifiers for two threads

    // Create two threads that will run 'thread_counter'
    pthread_create(&t1, NULL, thread_counter, NULL);
    pthread_create(&t2, NULL, thread_counter, NULL);

    // Wait for both threads to complete execution before continuing
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    // Print the final value of 'counter' after both threads have finished
    printf("Value of counter is %d\n", counter );

    return EXIT_SUCCESS; // Exit the program successfully
}
