#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char *argv[], char *envp[])
{
    // these are unused, (void) casting makes them "used"
    (void) argc;
    (void) argv;
    (void) envp;

    pid_t child_pid;
    char letter = 'w';

    // block before we've printed anything.
    // getchar();

    printf("Hello, world!\n");

    // block after we printed hello world
    // getchar();

    //pid_t parent = getpid();
    // on fork() our OS is going duplicate the current running
    // process (this one!), duplicate the "process control block"
    // for this process.
    child_pid = fork();
    // This makes it look like fork() "returns twice".
    // reality: two processes are currently running, both
    // are executing code starting from the same spot.

    if ( child_pid == 0 )
    {
        // we are executing in the child process.
        letter = 'c';
        printf("In child, my pid is %d\n", getpid());
        // don't care about the output from getchar(), I just
        // want the processes to block so I can inspect their
        // state with ps. 
        getchar();

        argv[0] = "cat";
        argv[1] = "process-management.c";
        argv[2] = NULL; // argv is a NULL-terminated array

        execvp( argv[0], argv );

        // execvp will replace our code segment (and effectively our entire
        // address space!) with the code segment from a different program
        // that means that we will never be able to get to the next lines
        // because the replaced code segment never "returns" to this code!
        printf("Hi, I'm still running code here\n");
        printf(":(, no, actually I'm not\n");
    }
    else
    {
        // become a responsible parent and wait for my
        // child to finish doing whatever it's doing.
        // blocking call:
        waitpid( child_pid, NULL, 0 );
        // will not proceed until process with
        // id child_pid has exited.

        // wait(NULL); // wait for *any* child to finish
    }

    printf("The pid returned from fork() is: %d\n", child_pid);
    printf("The value of letter is %c\n", letter);

    return EXIT_SUCCESS;
}
