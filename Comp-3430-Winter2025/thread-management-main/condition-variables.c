#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdatomic.h>

pthread_mutex_t cv_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  cv;     // Each Condition Variable must be used with a mutex(Lock)

atomic_uint threads_ready = 0;

pid_t gettid(void)
{
    return syscall(SYS_gettid);
}

void *work(void *args)
{
    (void) args;
    printf("Started thread %d.\n", gettid());

    // tell the main thread that we're ready (this is atomic now!)
    threads_ready++;

    // acquire the lock before trying to wait on the condition variable
    pthread_mutex_lock( &cv_lock );
    pthread_cond_wait( &cv, &cv_lock );
    // wait until the condition is signalled
    // (block, go to sleep); unlock the lock.
    //
    // when signalled, and passing this line
    // you hold the lock

    printf("Received signal in %d.\n", gettid());

    // do your work on the shared object
    
    
    pthread_mutex_unlock( &cv_lock );
    
    printf("Finished thread %d.\n", gettid());
    threads_ready--;
    return NULL;
}

int main(void)
{

#define NUM_THREADS 10
    pthread_t threads[NUM_THREADS];

    pthread_cond_init( &cv, NULL );
   
    // start everyone up
    for (int i = 0; i < NUM_THREADS; i++)
    {
        pthread_create(&threads[i], NULL, work, &i);
    }

    // spin or block until all threads have reported
    // themselves as ready
    while ( threads_ready < NUM_THREADS ) {}

    /*

    // This is the first Example shown in class -- LECTURE 22 -- This one makes sense

    while ( threads_ready > 0 )
    {
        printf("Press enter to signal a thread.");
        getchar();

        pthread_mutex_lock( &cv_lock );
        // wake up exactly one waiting thread
        pthread_cond_signal( &cv );
        pthread_mutex_unlock( &cv_lock );

    }*/

    printf("Press enter to signal all threads.");
        getchar();

        pthread_mutex_lock( &cv_lock );
        // wake up waiting threads
        pthread_cond_broadcast( &cv );
        pthread_mutex_unlock( &cv_lock );


    // wait for everyone to finish
    for (int i = 0; i < NUM_THREADS; i++)
    {
        pthread_join(threads[i], NULL);
    }

    return EXIT_SUCCESS;
}
