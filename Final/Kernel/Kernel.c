#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define ITERATIONS 100000

long diff_ns(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) * 1e9 + (end.tv_nsec - start.tv_nsec);
}

int main() {
    int pipe1[2], pipe2[2];
    pid_t pid;
    char buf = 'x';
    struct timespec start, end;

    pipe(pipe1);  // Parent to child
    pipe(pipe2);  // Child to parent

    pid = fork();

    if (pid == 0) {
        // Child
        for (int i = 0; i < ITERATIONS; ++i) {
            read(pipe1[0], &buf, 1);
            write(pipe2[1], &buf, 1);
        }
    } else {
        // Parent
        clock_gettime(CLOCK_MONOTONIC, &start);
        for (int i = 0; i < ITERATIONS; ++i) {
            write(pipe1[1], &buf, 1);
            read(pipe2[0], &buf, 1);
        }
        clock_gettime(CLOCK_MONOTONIC, &end);

        long total_ns = diff_ns(start, end);
        double avg_ns = (double) total_ns / (ITERATIONS * 2); // 2 switches per iteration

        printf("Total time: %ld ns for %d context switches\n", total_ns, ITERATIONS * 2);
        printf("Avg time per context switch: %.2f ns\n", avg_ns);
    }

    return 0;
}

