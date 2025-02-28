#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nqp_io.h"

#define MAX_LINE_SIZE 256
#define MAX_ARGS 10

// Global variable for current working directory (cwd)
char cwd[MAX_LINE_SIZE] = "/";

// Function prototypes
void handle_cd(char *dir);
void handle_pwd();
void handle_ls();

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
        if (arg_count == 0) continue;

        // ==========================
        // 🔥 Built-in Commands
        // ==========================
        if (strcmp(args[0], "exit") == 0) {
            printf("Exiting shell...\n");
            break;
        } 
        else if (strcmp(args[0], "cd") == 0) {
            if (arg_count < 2) {
                fprintf(stderr, "cd: missing argument\n");
            } else {
                handle_cd(args[1]); // Call cd function
            }
        } 
        else if (strcmp(args[0], "pwd") == 0) {
            handle_pwd(); // Call pwd function
        } 
        else if (strcmp(args[0], "ls") == 0) {
            handle_ls(); // Call ls function (uses Assignment 1's `ls.c`)
        } 
        else {
            printf("Unknown command: %s\n", args[0]);
        }
    }

    return EXIT_SUCCESS;
}

// ==========================
// 🔹 `cd` Implementation
// ==========================
void handle_cd(char *dir) {
    if (strcmp(dir, "..") == 0) {
        // Handle "cd .." (move to parent)
        if (strcmp(cwd, "/") != 0) {
            char *last_slash = strrchr(cwd, '/');
            if (last_slash != NULL) {
                *last_slash = '\0';
                if (strlen(cwd) == 0) strcpy(cwd, "/");
            }
        }
    } 
    else {
        // Handle "cd dir" (move into directory)
        char new_path[MAX_LINE_SIZE];
        snprintf(new_path, sizeof(new_path), "%s/%s", cwd, dir);

        if (nqp_is_directory(new_path)) {
            strcpy(cwd, new_path);  // Update cwd
        } else {
            fprintf(stderr, "%s: Directory not found or not a directory\n", dir);
        }
    }
}

// ==========================
// 🔹 `pwd` Implementation
// ==========================
void handle_pwd() {
    printf("%s\n", cwd);
}

// ==========================
// 🔹 `ls` Implementation (Uses `ls.c` From Assignment 1)
// ==========================
void handle_ls() {
    nqp_ls(cwd); // Assuming `nqp_ls()` is implemented in `ls.c`
}
