#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define SIZE 3
#define NUM_WORKERS 3

int A[SIZE][SIZE] = {
    {1, 2, 3},
    {4, 5, 6},
    {7, 8, 9}
};

int B[SIZE][SIZE] = {
    {9, 8, 7},
    {6, 5, 4},
    {3, 2, 1}
};

int C[SIZE][SIZE];

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

int current_row = -1;
int task_available = 0;

void multiply_row(int row) {
    for (int j = 0; j < SIZE; j++) {
        C[row][j] = 0;
        for (int k = 0; k < SIZE; k++) {
            C[row][j] += A[row][k] * B[k][j];
        }
    }
}

void* worker(void* arg) {
    int id = *(int*)arg;
    free(arg);

    while (1) {
        pthread_mutex_lock(&lock);
        while (!task_available) {
            pthread_cond_wait(&cond, &lock);
        }

        if (current_row >= SIZE) {
            pthread_mutex_unlock(&lock);
            break; // no more work
        }

        int row = current_row++;
        pthread_mutex_unlock(&lock);

        // Actual work
        printf("Thread %d working on row %d\n", id, row);
        multiply_row(row);
    }

    return NULL;
}

int main() {
    pthread_t threads[NUM_WORKERS];

    // Create threads
    for (int i = 0; i < NUM_WORKERS; i++) {
        int* arg = malloc(sizeof(int));
        *arg = i;
        pthread_create(&threads[i], NULL, worker, arg);
    }

    // Assign work
    pthread_mutex_lock(&lock);
    current_row = 0;
    task_available = 1;
    pthread_cond_broadcast(&cond); // wake up all threads
    pthread_mutex_unlock(&lock);

    // Wait for workers
    for (int i = 0; i < NUM_WORKERS; i++) {
        pthread_join(threads[i], NULL);
    }

    // Show result
    printf("\nResult matrix C:\n");
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            printf("%4d ", C[i][j]);
        }
        printf("\n");
    }

    pthread_mutex_destroy(&lock);
    pthread_cond_destroy(&cond);

    return 0;
}

