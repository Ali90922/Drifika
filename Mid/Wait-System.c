#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>

int main(void) {
    pid_t pid;

    pid = fork();
    if (pid == 0) {
        // Child process
        printf("Child process (PID %d): Working for 3 seconds...\n", getpid());
        sleep(3);
        printf("Child process (PID %d): Exiting with code 7.\n", getpid());
        exit(7);
    } else if (pid > 0) {
        // Parent process: Using waitid to get detailed info
        siginfo_t info;
        int ret = waitid(P_PID, pid, &info, WEXITED);
        if (ret != 0) {
            perror("waitid");
            exit(EXIT_FAILURE);
        }
        // Check the signal info structure
        if (info.si_code == CLD_EXITED) {
            printf("Parent: Child (PID %d) exited normally with code %d.\n",
                   info.si_pid, info.si_status);
        } else {
            printf("Parent: Child (PID %d) did not exit normally.\n", info.si_pid);
        }
    } else {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    return 0;
}

