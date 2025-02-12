#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    // Create a new process using fork()
    pid_t pid = fork();
    
    if (pid < 0) {
        // fork() returns a negative value if creation fails
        perror("fork failed");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Child process block
        printf("Child process: PID = %d, Parent PID = %d\n", getpid(), getppid());
        
        // Replace the child process image with a new program, e.g., listing directory contents
        execlp("ls", "ls", "-l", (char *)NULL);
        
        // If exec fails, report error and exit the child process
        perror("execlp failed");
        exit(EXIT_FAILURE);
    } else {
        // Parent process block
        printf("Parent process: Waiting for child process %d to complete.\n", pid);
        
        // Wait for the child process to finish and collect its exit status
        int status;
        waitpid(pid, &status, 0);
        
        if (WIFEXITED(status)) {
            printf("Child process exited with status %d\n", WEXITSTATUS(status));
        } else {
            printf("Child process did not terminate normally.\n");
        }
    }
    
    return 0;
}

