#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

int flag = 0;

void handle_signal(int signum, siginfo_t *info, void *ucontext)
{
    (void) ucontext; // let's just pretend this doesn't exist.

    printf("Handling signal: %d\n", signum);
    // Question: when we sent signals with the `kill` command, the 
    // value of info->si_pid changed every time we did it. Why?
    printf("Received from: %d\n", info->si_pid );
    // set the flag:
    flag = 1;
}

int main(void) 
{
    struct
    {
        // this is a bitfield. it's limited to two bits and has the handy
        // property that it will overflow back to zero (because it's unsigned).
        unsigned int count:2;
    } counter;

    char indicators[] = {'-', '\\', '|', '/'};

    pid_t mypid = getpid();
    pid_t child_pid = -1, parent_pid = -1;

    printf("My pid is %d\n", mypid);

    struct sigaction handler = {
        .sa_sigaction = handle_signal,
        .sa_flags = SA_SIGINFO
    };

    sigaction( SIGINT, &handler, NULL );

    // Question: we registered this, but we still got killed when the SIGKILL
    // signal was sent. Why didn't our handler replace the one for SIGKILL?
    sigaction( SIGKILL, &handler, NULL );


    child_pid = fork();

    if ( child_pid == 0 )
    {
        // mypid is going to be equal to this,
        // we called getpid() before fork();
        parent_pid = getppid();
        printf("In child, about to do hard work.\n");
        // we're in the child process
        // do some "real hard work"
        sleep( 5 );
        // tell my parent process that I'm done my
        // hard work, but I'm not exited.
        kill( parent_pid, SIGINT );
        printf("Finished telling parent about my hard work\n");
        // some more hard work:
        sleep( 3 );
        exit( EXIT_SUCCESS );
    }

    // sit around spinning until the flag is set.
    // effectively a blocking operation until the
    // flag is set.
	while ( !flag )
    {
        write(STDOUT_FILENO, "\r", 1);
        write(STDOUT_FILENO, &indicators[counter.count++], 1);
        usleep(250000);
    }

    printf("Child says hard work is done, going to wait"
           " for child to exit\n");
    // wait now for any child (we have just one)
    wait( NULL );
    printf("All done, bye!\n");

	return EXIT_SUCCESS;
}

