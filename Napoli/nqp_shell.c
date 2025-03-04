#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE

#include <fcntl.h>  // For mkstemp
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
        printf("%s:\\> ", cwd);
        if (fgets(line_buffer, MAX_LINE_SIZE, stdin) == NULL) {
            printf("\nExiting shell...\n");
            break;
        }
        line_buffer[strcspn(line_buffer, "\n")] = '\0';

        char *token = strtok(line_buffer, " ");
        int arg_count = 0;
        while (token != NULL && arg_count < MAX_ARGS - 1) {
            args[arg_count++] = token;
            token = strtok(NULL, " ");
        }
        args[arg_count] = NULL;
        if (arg_count == 0)
            continue;

        if (strcmp(args[0], "exit") == 0) {
            printf("Exiting shell...\n");
            break;
        } else if (strcmp(args[0], "pwd") == 0) {
            handle_pwd();
        } else if (strcmp(args[0], "ls") == 0) {
            handle_ls();
        } else if (strcmp(args[0], "cd") == 0) {
            handle_cd(args[1]);
        } else {
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
    if (strcmp(cwd, "/") == 0) {
        if (Argument1[0] != '/') {
            snprintf(abs_path, sizeof(abs_path), "/%s", Argument1);
        } else {
            strncpy(abs_path, Argument1, sizeof(abs_path));
        }
    } else {
        snprintf(abs_path, sizeof(abs_path), "%s/%s", cwd, Argument1);
    }

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

    // Set execute permissions on the in-memory file.
    if (fchmod(InMemoryFile, 0755) == -1) {
        perror("fchmod");
        return;
    }

    // Reset the in-memory file offset before debugging.
    if (lseek(InMemoryFile, 0, SEEK_SET) == -1) {
        perror("lseek before header debug");
        return;
    }

    // Debug: Read and print the first 16 bytes.
    unsigned char debug_header[16];
    ssize_t n = read(InMemoryFile, debug_header, sizeof(debug_header));
    if (n != sizeof(debug_header)) {
        perror("read header");
        return;
    }
    printf("In-memory file header: ");
    for (size_t i = 0; i < sizeof(debug_header); i++) {
        printf("%02x ", debug_header[i]);
    }
    printf("\n");
    fflush(stdout);

    // Reset offset again before execution.
    if (lseek(InMemoryFile, 0, SEEK_SET) == -1) {
        perror("lseek after header debug");
        return;
    }

    char *argv[] = { Argument1, Argument2, NULL };
    char *envp[] = { NULL };

    // Check if the file is a shell script by looking for "#!" at the start.
    if (debug_header[0] == '#' && debug_header[1] == '!') {
        printf("Detected shell script, using temporary file workaround\n");
        fflush(stdout);

        // Create a temporary file that will remain on disk.
        char tmp_template[] = "/tmp/scriptXXXXXX";
        int tmp_fd = mkstemp(tmp_template);
        if (tmp_fd == -1) {
            perror("mkstemp");
            exit(1);
        }
        // Do NOT unlink the temporary file; we need its name for execve.

        // Copy the entire content from the in-memory file to the temporary file.
        if (lseek(InMemoryFile, 0, SEEK_SET) == -1) {
            perror("lseek before copying to tmp");
            exit(1);
        }
        while ((bytes_read = read(InMemoryFile, buffer, BUFFER_SIZE)) > 0) {
            if (write(tmp_fd, buffer, bytes_read) != bytes_read) {
                perror("write to tmp file");
                exit(1);
            }
        }
        if (bytes_read < 0) {
            perror("read from in-memory file");
            exit(1);
        }
        // Set execute permissions on the temporary file.
        if (fchmod(tmp_fd, 0755) == -1) {
            perror("fchmod tmp file");
            exit(1);
        }
        close(tmp_fd);

        // Fork and execute using execve on the temporary file.
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            exit(1);
        }
        if (pid == 0) {
            // Change the working directory to cwd so relative paths work.
            if (chdir(cwd) == -1) {
                perror("chdir");
                exit(1);
            }
            // Build the argv vector for the shell script.
            char *tmp_argv[] = { tmp_template, Argument2, NULL };
            if (execve(tmp_template, tmp_argv, envp) == -1) {
                perror("execve");
                exit(1);
            }
        } else {
            int status;
            waitpid(pid, &status, 0);
            // Optionally, remove the temporary file after execution.
            // unlink(tmp_template);
        }
    } else {
        // Otherwise, assume it's a proper ELF binary.
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            return;
        }
        if (pid == 0) {
            if (fexecve(InMemoryFile, argv, envp) == -1) {
                perror("fexecve");
                exit(1);
            }
        } else {
            int status;
            waitpid(pid, &status, 0);
        }
    }
}
