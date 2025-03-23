#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

void *thread_counter(void *args)
{
    (void) args;

    int *counter = malloc(sizeof(int));

    *counter = 0;

    for (int i = 0; i < 1000000; i++)
    {
        (*counter)++;
    }
    return counter;
}

int main(void)
{
    pthread_t t1, t2;

    int counter = 0;
    int *t1_counter, *t2_counter;

    t1_counter = malloc(sizeof(int));
    t2_counter = malloc(sizeof(int));

    pthread_create( &t1, NULL, thread_counter, NULL );
    pthread_create( &t2, NULL, thread_counter, NULL );

    pthread_join( t1, (void **)&t1_counter );
    pthread_join( t2, (void **)&t2_counter );

    counter = *t1_counter + *t2_counter;

    printf("Value of counter is %d\n", counter );

    return EXIT_SUCCESS;
}
