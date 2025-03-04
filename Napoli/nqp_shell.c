#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nqp_io.h"
#include <sys/stat.h>

#define BUFFER_SIZE 1024
#define MAX_LINE_SIZE 256
#define MAX_ARGS 10

// Global variable for current working directory (cwd)
char cwd[MAX_LINE_SIZE] = "/";

// Function prototypes
void handle_cd(char *dir);
void handle_pwd();
void handle_ls();
void LaunchFunction(char *Argument1, char *Argument2);

int main(int argc, char *argv[], char *envp[]) {
    char line_buffer[MAX_LINE_SIZE] = {0};
    char *args[MAX_ARGS];
    nqp_error mount_error;

    (void)envp; // Unused

    if (argc != 2) {
        fprintf(stderr, "Usage: ./nqp_shell volume.img\n");
        exit(EXIT_FAILURE);
    }

    // Mount exFAT filesystem
    mount_error = nqp_mount(argv[1], NQP_FS_EXFAT);
    if (mount_error != NQP_OK) {
        if (mount_error == NQP_FSCK_FAIL) {
            fprintf(stderr, "%s is inconsistent, not mounting.\n", argv[1]);
        }
        exit(EXIT_FAILURE);
    }

    // Shell loop
    while (1) {
        printf("%s:\\> ", cwd); // Display prompt
        if (fgets(line_buffer, MAX_LINE_SIZE, stdin) == NULL) {
            printf("\nExiting shell...\n");
            break;
        }
        // Remove newline character
        line_buffer[strcspn(line_buffer, "\n")] = '\0';

        // Tokenize input
        char *token = strtok(line_buffer, " ");
        int arg_count = 0;
        while (token != NULL && arg_count < MAX_ARGS - 1) {
            args[arg_count++] = token;
            token = strtok(NULL, " ");
        }
        args[arg_count] = NULL;  // Null-terminate argument list

        // Handle empty input
        if (arg_count == 0)
            continue;

        if (strcmp(args[0], "exit") == 0) {
            printf("Exiting shell...\n");
            break;
        } 
        else if (strcmp(args[0], "pwd") == 0) {
            handle_pwd();
        } 
        else if (strcmp(args[0], "ls") == 0) {
            handle_ls();
        }
        else if (strcmp(args[0], "cd") == 0) {
            handle_cd(args[1]);
        }
        else {
            LaunchFunction(args[0], args[1]);
        }
    }

    return EXIT_SUCCESS;
}

void handle_pwd() {
    printf("%s\n", cwd);
}

void handle_cd(char *dir) {
    char path_copy[256];
    strncpy(path_copy, dir, sizeof(path_copy));
    if (nqp_open(dir) != -1) {
        strcpy(cwd, path_copy);
    }
}

void handle_ls() {
    nqp_dirent entry = {0};
    int fd;
    ssize_t dirents_read;
    fd = nqp_open(cwd);
    if (fd == NQP_FILE_NOT_FOUND) {
        fprintf(stderr, "%s not found\n", cwd);
    } else {
        while ((dirents_read = nqp_getdents(fd, &entry, 1)) > 0) {
            printf("%lu %s", entry.inode_number, entry.name);
            if (entry.type == DT_DIR)
                putchar('/');
            putchar('\n');
            free(entry.name);
        }
        if (dirents_read == -1)
            fprintf(stderr, "%s is not a directory\n", cwd);
        nqp_close(fd);
    }
}

void LaunchFunction(char *Argument1, char *Argument2) {
    int exec_fd = 0;
    char abs_path[MAX_LINE_SIZE];

    // Build the absolute path for the command.
    snprintf(abs_path, sizeof(abs_path), "/%s", Argument1);

    exec_fd = nqp_open(abs_path);
    if (exec_fd == NQP_FILE_NOT_FOUND) {
        fprintf(stderr, "Command %s not found\n", abs_path);
        return;
    }
    printf("File Descriptor for command (%s) is : %d\n", abs_path, exec_fd);

    // Create an in-memory file descriptor.
    int InMemoryFile = memfd_create("In-Memory-File", MFD_CLOEXEC);
    printf("File Descriptor for In Memory File is  : %d\n", InMemoryFile);
    if (InMemoryFile == -1) {
        perror("memfd_create");
        return;
    }

    // Copy executable data from the source file into the in-memory file.
    ssize_t bytes_read, bytes_written;
    char buffer[BUFFER_SIZE];
    size_t total_bytes = 0;
    while ((bytes_read = nqp_read(exec_fd, buffer, BUFFER_SIZE)) > 0) {
        total_bytes += bytes_read;
        bytes_written = write(InMemoryFile, buffer, bytes_read);
        if (bytes_written != bytes_read) {
            fprintf(stderr, "Error writing to in-memory file\n");
            return;
        }
    }
    if (bytes_read < 0) {
        fprintf(stderr, "Error reading the source file\n");
        return;
    }
    printf("Total bytes read from source: %zu\n", total_bytes);

    // Read first two bytes to detect if it's a shell script
    char header[2];
    lseek(InMemoryFile, 0, SEEK_SET);
    read(InMemoryFile, header, 2);
    lseek(InMemoryFile, 0, SEEK_SET);

    char *argv[] = { Argument1, Argument2, NULL };
    char *envp[] = { NULL };

    // If the file starts with "#!", execute it via /bin/sh
    if (header[0] == '#' && header[1] == '!') {
        printf("Detected shell script, executing with /bin/sh\n");
        argv[0] = "/bin/sh";
        argv[1] = abs_path;
        argv[2] = NULL;
        if (fexecve(InMemoryFile, argv, envp) == -1) {
            perror("fexecve");
            return;
        }
    }

    // Otherwise, assume it's an ELF binary
    if (fexecve(InMemoryFile, argv, envp) == -1) {
        perror("fexecve");
        return;
    }
}
