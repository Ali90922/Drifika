#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <openssl/sha.h>

// How to run : 
//gcc file_hash_logger.c -o file_hash_logger -pthread -lssl -lcrypto

#define NUM_FILES 3

const char* filenames[] = {"file1.txt", "file2.txt", "file3.txt"};

pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t stats_mutex = PTHREAD_MUTEX_INITIALIZER;

int total_bytes = 0; // shared variable (unsafe!)
FILE* logfile;

void compute_sha256(const char* path, char* output) {
    FILE* file = fopen(path, "rb");
    if (!file) {
        perror("File open error");
        strcpy(output, "ERROR");
        return;
    }

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);

    char buf[1024];
    int bytes;
    while ((bytes = fread(buf, 1, sizeof(buf), file))) {
        SHA256_Update(&sha256, buf, bytes);
    }

    SHA256_Final(hash, &sha256);
    fclose(file);

    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        sprintf(output + (i * 2), "%02x", hash[i]);
    }
}

void* worker(void* arg) {
    const char* filename = (const char*)arg;
    char hash_output[65] = {0};

    // Simulate bad lock order for deadlock
    pthread_mutex_lock(&stats_mutex);
    sleep(1); // artificial delay
    pthread_mutex_lock(&log_mutex);

    // CRITICAL SECTION: compute + write log
    compute_sha256(filename, hash_output);

    FILE* f = fopen(filename, "rb");
    fseek(f, 0, SEEK_END);
    int size = ftell(f);
    fclose(f);

    fprintf(logfile, "[%s] Hash: %s | Size: %d bytes\n", filename, hash_output, size);

    // Race condition here if not protected!
    total_bytes += size;

    pthread_mutex_unlock(&log_mutex);
    pthread_mutex_unlock(&stats_mutex);

    return NULL;
}

int main() {
    pthread_t threads[NUM_FILES];

    logfile = fopen("hash_log.txt", "w");
    if (!logfile) {
        perror("Cannot open log file");
        return 1;
    }

    // Spawn threads
    for (int i = 0; i < NUM_FILES; ++i) {
        pthread_create(&threads[i], NULL, worker, (void*)filenames[i]);
    }

    // Wait for all
    for (int i = 0; i < NUM_FILES; ++i) {
        pthread_join(threads[i], NULL);
    }

    fclose(logfile);

    printf("\n✅ Done! Total bytes read (may be wrong if race hit): %d\n", total_bytes);
    return 0;
}

