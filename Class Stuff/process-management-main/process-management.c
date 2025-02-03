#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[], char *envp[])
{
    // these are unused, (void) casting makes them "used"
    (void) argc;
    (void) argv;
    (void) envp;

    pid_t pid;
    char letter = 'w';


    // block before we've printed anything.
    // getchar();

    printf("Hello, world!\n");

    // block after we printed hello world
    // getchar();

    // on fork() our OS is going duplicate the current running
    // process (this one!), duplicate the "process control block"
    // for this process.
    pid = fork();
    // This makes it look like fork() "returns twice".
    // reality: two processes are currently running, both
    // are executing code starting from the same spot.

    if ( pid == 0 )
    {
        // we are executing in the child process.
        letter = 'c';
        printf("In child, my pid is %d\n", getpid());
    }

    // don't care about the output from getchar(), I just
    // want the processes to block so I can inspect their
    // state with ps. we need to press Enter *twice* for
    // both processes to exit.
    getchar();

    printf("The pid returned from fork() is: %d\n", pid);
    printf("The value of letter is %c\n", letter);



    return EXIT_SUCCESS;
}
