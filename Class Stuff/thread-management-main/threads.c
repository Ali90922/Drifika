#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/wait.h>
#include <stdint.h>
#include <fcntl.h>
#include <pthread.h>

// Global variable 'c' that will store a character input from the user.
char c = '\0';

// Function executed by the newly created thread.
void *thread_work(void *args)
{
    // Unused argument, marked to avoid compiler warnings.
    (void) args;

    // Display the process ID of the thread and prompt the user for input.
    printf("[%d] In child, enter another letter, press Ctrl+D: ", getpid());

    // Read a character from standard input.
    c = getchar();
    
    // Print a newline for formatting.
    putchar('\n');

    // Return NULL as the thread's exit value (no meaningful return data).
    return NULL;
}

int main(int argc, char *argv[], char *envp[])
{
    // Marking unused arguments to avoid compiler warnings.
    (void) argc;
    (void) argv;
    (void) envp;

    // Declare a thread variable to hold the thread identifier.
    pthread_t child_thread;

    // Prompt the user for input before creating the new thread.
    printf("[%d] Before pthread_create(), enter a letter, press Ctrl+D: ", getpid());

    // Read a character from standard input.
    c = getchar();
    
    // Print a newline for formatting.
    putchar('\n');
    
    /*
     * Instead of using fork(), which creates a new process, we are using pthread_create().
     * 
     * A process has a single Process Control Block (PCB), which contains execution contexts.
     * Before calling pthread_create, there is only one execution context (main thread).
     * After calling pthread_create, there are now two execution contexts (main thread + child thread).
     * 
     * The new thread will execute thread_work(), and it will share the same address space as the main thread.
     */
    pthread_create(&child_thread, NULL, thread_work, NULL);

    /*
     * The original code contained a commented-out fork() section.
     * 
     * If we had used fork(), it would have duplicated the entire process, 
     * creating a new process with a separate memory space.
     * The child process would have executed its code independently of the parent.
     * However, using threads is more lightweight than creating a separate process.
     */

    // The main thread waits for the child thread to complete.
    printf("[%d] In parent, waiting for thread child.\n", getpid());

    // pthread_join ensures the main thread waits for the child thread to finish execution.
    pthread_join(child_thread, NULL);

    /*
     * After pthread_join(), there is only one execution context remaining (the main thread).
     * The child thread has completed its work and exited.
     */

    // Print the final value of 'c' after both inputs (one from main thread, one from child thread).
    printf("[%d] value of c is [%c]; All done!\n", getpid(), c);

    // Return success exit code.
    return EXIT_SUCCESS;
}
