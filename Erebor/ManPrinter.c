#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include <stdint.h>
#include <sched.h>

void* printer(void *arg)
{
    assert(arg != NULL);

    uint64_t counter = 0;
    char letter = *(char *)arg;

    while (counter < 200)
    {
        printf("%c %llu\n", letter, (unsigned long long)counter);
        counter++;

        if (counter % 50 == 0)
        {
            sched_yield();
        }
    }

    pthread_exit(NULL);
}

int main(void)
{
    char *letters = "ab";
    pthread_t thread_a, thread_b;

    if (pthread_create(&thread_a, NULL, printer, &letters[0]) != 0)
    {
        perror("Failed to create thread_a");
        exit(EXIT_FAILURE);
    }

    if (pthread_create(&thread_b, NULL, printer, &letters[1]) != 0)
    {
        perror("Failed to create thread_b");
        exit(EXIT_FAILURE);
    }

    pthread_join(thread_a, NULL);
    pthread_join(thread_b, NULL);

    return EXIT_SUCCESS;
}

