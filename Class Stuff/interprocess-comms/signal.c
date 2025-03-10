#include <stdio.h>      // for printf()
#include <time.h>       // for time-related functions (not used in this example)
#include <stdlib.h>     // for exit(), EXIT_SUCCESS, EXIT_FAILURE
#include <unistd.h>     // for fork(), sleep(), usleep(), write(), getpid(), getppid()
#include <signal.h>     // for signal handling functions and definitions
#include <sys/wait.h>   // for wait()

// Global flag that will be set when a signal is handled.
// The parent process uses this flag to determine when to stop the spinner.
int flag = 0;

/*
 * Signal handler function that will be called when a signal is received.
 *
 * Parameters:
 *   - signum: the signal number received (e.g., SIGINT).
 *   - info: a pointer to a siginfo_t structure that contains extra information about the signal.
 *           For example, info->si_pid holds the sender's process ID.
 *   - ucontext: a pointer to a ucontext_t structure (unused in this example).
 */
void handle_signal(int signum, siginfo_t *info, void *ucontext)
{
    // Prevent unused variable warning for ucontext.
    (void) ucontext;

    // Print the signal number received.
    printf("Handling signal: %d\n", signum);
    // Print the sender's PID from the siginfo_t structure.
    // Note: If you use the kill command multiple times, each invocation comes from a different process,
    //       so info->si_pid will change each time.
    printf("Received from: %d\n", info->si_pid);

    // Set the flag to 1 to indicate that a signal has been received.
    flag = 1;
}

int main(void) 
{
    // Define a structure with a 2-bit wide counter.
    // The 2-bit field means the value can only range from 0 to 3.
    // When it increments past 3, it automatically wraps back to 0.
    struct {
        unsigned int count:2;
    } counter;

    // An array of characters used to display a simple spinner animation.
    char indicators[] = {'-', '\\', '|', '/'};

    // Get the process ID of the current process.
    pid_t mypid = getpid();
    // Variables to store the child's and parent's process IDs.
    pid_t child_pid = -1, parent_pid = -1;

    // Print the current process ID.
    printf("My pid is %d\n", mypid);

    // Set up a sigaction structure to define the behavior for handling signals.
    // We use sa_sigaction because we want to receive extended signal information.
    struct sigaction handler = {
        .sa_sigaction = handle_signal,  // Our custom signal handler function.
        .sa_flags = SA_SIGINFO           // Flag to indicate that we want extra info in the handler.
    };

    // Register our signal handler for SIGINT.
    // When SIGINT (Ctrl+C or sent by kill) is received, handle_signal will be called.
    sigaction(SIGINT, &handler, NULL);

    // Attempt to register our handler for SIGKILL.
    // Note: SIGKILL cannot be caught, blocked, or handled; it always terminates the process immediately.
    sigaction(SIGKILL, &handler, NULL);

    // Create a new process using fork().
    // In the child process, fork() returns 0.
    // In the parent process, fork() returns the PID of the newly created child.
    child_pid = fork();

    if (child_pid == 0)
    {
        // This code runs in the child process.

        // Get the parent's process ID.
        parent_pid = getppid();
        printf("In child, about to do hard work.\n");

        // Simulate doing some "hard work" by sleeping for 5 seconds.
        sleep(5);

        // After finishing the work, send a SIGINT signal to the parent process.
        // This notifies the parent that the child has completed its hard work.
        kill(parent_pid, SIGINT);

        // Print confirmation that the signal has been sent.
        printf("Finished telling parent about my hard work\n");

        // Simulate doing a bit more work by sleeping for 3 more seconds.
        sleep(3);

        // Exit the child process with a success status.
        exit(EXIT_SUCCESS);
    }

    // This code runs in the parent process.
    // Enter a loop that displays a spinner animation until the signal is received.
    while (!flag)
    {
        // Write a carriage return to bring the cursor back to the beginning of the line.
        write(STDOUT_FILENO, "\r", 1);

        // Write the current spinner character to the terminal.
        // The counter.count value automatically wraps from 3 back to 0 because it is a 2-bit field.
        write(STDOUT_FILENO, &indicators[counter.count++], 1);

        // Sleep for 250,000 microseconds (0.25 seconds) to control the animation speed.
        usleep(250000);
    }

    // Once the flag is set (i.e., the signal has been handled), notify that the child has completed its work.
    printf("Child says hard work is done, going to wait for child to exit\n");

    // Wait for the child process to terminate.
    // This ensures that the parent does not exit before the child, avoiding a zombie process.
    wait(NULL);

    // Print a final message indicating that the process is done.
    printf("All done, bye!\n");

    // Exit the parent process with a success status.
    return EXIT_SUCCESS;
}
