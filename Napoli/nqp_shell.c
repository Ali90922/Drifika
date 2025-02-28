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
// 🔹 `pwd` Implementation
// ==========================
void handle_pwd() {
    printf("%s\n", cwd);
}

// ==========================
// 🔹 `ls` Implementation (Uses `ls.c` From Assignment 1)
// ==========================
void handle_ls() {
    fd = nqp_open(cwd);
    if (fd == NQP_FILE_NOT_FOUND){
                fprintf(stderr, "%s not found\n", argv[2]);
            }else
            {
                while ((dirents_read = nqp_getdents(fd, &entry, 1)) > 0)
                {
                    printf("%lu %s", entry.inode_number, entry.name);

                    if (entry.type == DT_DIR)
                    {
                        putchar('/');
                    }

                    putchar('\n');

                    free(entry.name);
                }

                if (dirents_read == -1)
                {
                    fprintf(stderr, "%s is not a directory\n", argv[2]);
                }

                nqp_close(fd);
            }
    
    
}
