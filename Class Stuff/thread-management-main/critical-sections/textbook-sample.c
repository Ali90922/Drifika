#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

int counter = 0;

// in this specific case must be shared and accessible
// by all threads that are making changes to the resource
// that they are sharing.
pthread_mutex_t counter_lock = PTHREAD_MUTEX_INITIALIZER;

void *thread_counter(void *args)
{
    (void) args;
    pthread_mutex_lock(&counter_lock);
    for (int i = 0; i < 1000000; i++)
    {
        // guaranteed that only one thread will
        // execute the statements inside the locked
        // area at a time.
        // this is now effectively atomic.
        //pthread_mutex_lock(&counter_lock);
        counter++;
        //pthread_mutex_unlock(&counter_lock);
    }  
    pthread_mutex_unlock(&counter_lock);
    return NULL;
}

int main(void)
{
    pthread_t t1, t2;

    pthread_create( &t1, NULL, thread_counter, NULL );
    pthread_create( &t2, NULL, thread_counter, NULL );

    pthread_join( t1, NULL );
    pthread_join( t2, NULL );

    printf("Value of counter is %d\n", counter );

    return EXIT_SUCCESS;
}
