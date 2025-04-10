#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/wait.h>

int flag = 0;

void increment_flag( int signum, siginfo_t *info, void *ucontext )
{
    (void) ucontext;
    (void) info;
    (void) signum;
    flag++;
}

int main(void)
{
    int fds[2] = {0};
    pid_t child_pid = -1;
    uint8_t byte;

    pipe( fds );
    struct sigaction child_ready = {
        .sa_sigaction = increment_flag,
        .sa_flags = SA_SIGINFO
    };
    (void) child_ready;

    // theory: maybe read is happening after write.
    // let's use signals to synchronize the parent
    // and child's call to write/read.
    // IMPORTANT: this is not the right path.
    sigaction( SIGUSR1, &child_ready, NULL );
    
    child_pid = fork();
    if (child_pid == 0)
    {
        close( fds[1] ); // close the write end
                         // of the pipe so I don't
                         // get stuck waiting for
                         // writers on the pipe
        printf("Waiting for message from parent.\n");
        kill( getppid(), SIGUSR1 );
        while( read( fds[0], &byte, 1 ) > 0 )
        {
            // NOTE: putchar and all of stdio will
            // buffer input until a newline character
            // is printed.
            // putchar( byte );
            write( STDOUT_FILENO, &byte, 1 );
        }

        exit( EXIT_SUCCESS );
    }

    close( fds[0] ); // parent process never reads

    printf("Waiting for child to be ready.\n");
    while( flag == 0 ) {}

    printf("About to write to the child\n");
    write( fds[1], "hi!", 3 );

    close( fds[1] ); // I'm done writing, let
                     // any reader know I'm done.

    printf("Message sent, waiting for child to end.\n");
    wait( NULL );
    
    return EXIT_SUCCESS;
}
