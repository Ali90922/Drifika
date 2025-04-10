#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/wait.h>
#include <stdint.h>
#include <fcntl.h>
#include <pthread.h>

char c = '\0';

void *thread_work(void *args)
{
    (void) args;

    printf("[%d] In child, enter another letter, press Ctrl+D: ", getpid());
    c = getchar();
    putchar('\n');

    return NULL;
}

int main(int argc, char *argv[], char *envp[])
{
    (void) argc;
    (void) argv;
    (void) envp;

    // pid_t child_pid;
    pthread_t child_thread;

    printf("[%d] Before pthread_create(), enter a letter, press Ctrl+D: ", getpid());
    c = getchar();
    putchar('\n');
    
    // duplicate our PCB, including the open file table
    // child_pid = fork();

    // Please create a new context, for a new thread of execution.
    // context: registers (including program counter) + stack.
    // New context will be placed into the process control block.
    // We have a list of contexts in the PCB.
    // Before calling pthread_create we had 1 contexts, after
    // we have 2 contexts.
    pthread_create( &child_thread, NULL, thread_work, NULL );

    /*
    if ( child_pid == 0 )
    {
        printf("[%d] In child, enter another letter, press Ctrl+D: ", getpid());
        c = getchar();
        putchar('\n');
    }
    else
    {
        printf("[%d] In parent, waiting for child.\n", getpid());
        // in the parent process, parent process will never
        // read() from the pipe.
        wait( NULL );
    }*/

    printf("[%d] In parent, waiting for thread child.\n", getpid());
    pthread_join( child_thread, NULL );
    // after pthread_join there is only one context
    // or one thread of execution (the *main* thread of execution).

    printf("[%d] value of c is [%c]; All done!\n", getpid(), c );

    return EXIT_SUCCESS;
}
