#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

int outside_counter = 0;

void *thread_counter(void *args)
{
    (void) args;

    int counter = 0; // this is "thread-local"; it's a
                     // variable, value, memory location
                     // that's contained within this
                     // thread's stack.
    for (int i = 0; i < 1000000; i++)
    {
        counter++;
    }
    (void) counter; // oops.
    return NULL;
}

int main(void)
{
    pthread_t t1, t2;

    pthread_create( &t1, NULL, thread_counter, NULL );
    pthread_create( &t2, NULL, thread_counter, NULL );

    pthread_join( t1, NULL );
    pthread_join( t2, NULL );

    printf("Value of counter is %d\n", outside_counter );

    return EXIT_SUCCESS;
}
