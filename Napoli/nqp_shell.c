#define _GNU_SOURCE
#include <sys/mman.h> // Include for memfd_create and MFD_CLOEXEC
#include <unistd.h> //include for memfd_create
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nqp_io.h"



#define BUFFER_SIZE 1024
#define MAX_LINE_SIZE 256
#define MAX_ARGS 10


// Tasks 

// Built In Tasks :
// 1 . Implement ls -- Function Copied over from the last assignment and adjusted as needed -- DONE
// 2 pwd . -- Used the global variable for this and just need to print it    -- DONE
// 3 -- CD -- Need to implement change of current working directory -- look at code from the earlier assignment ! -- DONE

// Launch Process Tasks : 



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
        if (arg_count == 0) continue;

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
        else if(strcmp(args[0], "cd") == 0){
            handle_cd(args[1]);
        }
        else {
            LaunchFunction(args[0],args[1]);
        }
    }

    return EXIT_SUCCESS;
}



void handle_pwd() {
    printf("%s\n", cwd);
}



void handle_cd(char *dir){
    char path_copy[256];
    strncpy(path_copy, dir, sizeof(path_copy)); // Copy into char
    //char *token = strtok(path_copy, "/"); // Tokenization
    if(nqp_open(dir)!= -1){
        strcpy(cwd, path_copy);
    }
}


void handle_ls() {
    nqp_dirent entry = {0};
    int fd;
    ssize_t dirents_read;
    fd = nqp_open(cwd);
    if (fd == NQP_FILE_NOT_FOUND){
                fprintf(stderr, "%s not found\n", cwd);
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
                    fprintf(stderr, "%s is not a directory\n", cwd);
                }

                nqp_close(fd);
            }
}

/*
Your shell should be able to launch the programs that are installed on the volume that has been provided. The OS that our shell is running on actually has no idea how to read data (including programs!) out of the volume unless we mount-ed the volume for realsies, but neither me (Franklin) nor you (student) have permission to mount volumes on shared systems like Aviary.

We are going to be using POSIX system calls (ones that would run on macOS, Linux, Solaris, etc) like fork and wait and also Linux-specific system calls (ones that only run on Linux) like memfd_create and fexecve.

Your shell should be able to take a command that has been entered, find the corresponding command in the current working directory, read that command into memory (using memfd_create and both nqp_open and nqp_read), then execute that command (using a combination of fork and fexecve).

From the root directory of the volume, we should able to run the following command and see the following output:

3430:\> echo "Hi!"
Hi!
3430:\> _


*/

void LaunchFunction(char *Argument1, char *Argument2) {
    int FileDescriptor = 0;

    // Open the file for the command (assuming cwd is "/" for now)
    if (strcmp(cwd, "/") == 0) {
        FileDescriptor = nqp_open(Argument1);
        printf("File Descriptor is : %d\n", FileDescriptor);
    } else {
        // Append filename to cwd and then open, if necessary.
    }

    // Create an in-memory file descriptor
    int InMemoryFile = memfd_create("In-Memory-File", MFD_CLOEXEC);
    if (InMemoryFile == -1) {
        perror("memfd_create");
        return;
    }

    // Read bytes from the source file and write them into the in-memory file
    ssize_t bytes_read, bytes_written;
    char buffer[BUFFER_SIZE];
    while ((bytes_read = nqp_read(FileDescriptor, buffer, BUFFER_SIZE)) > 0) {
        bytes_written = write(InMemoryFile, buffer, bytes_read);
        if (bytes_written != bytes_read) {
            fprintf(stderr, "Error writing to in-memory file\n");
            // Handle error appropriately
        }
    }
    if (bytes_read < 0) {
        fprintf(stderr, "Error reading the source file\n");
        // Handle error appropriately
    }

    // IMPORTANT: Reset the file offset to the beginning before fexecve
    if (lseek(InMemoryFile, 0, SEEK_SET) == -1) {
        perror("lseek");
        return;
    }

    // Fork and execute the command using fexecve in the child process
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        return;
    }
    if (pid == 0) {
        // Child Process
        char *envp[] = { NULL };
        // Construct argv; typically, argv[0] is the program name
        char *argv[] = { Argument2, NULL };
        lseek(InMemoryFile, 0, SEEK_SET);
        if (fexecve(9, argv, envp) == -1) {
            perror("fexecve");
            exit(1);
        }
    }

    fd = nqp_open(9);

            if (fd == NQP_FILE_NOT_FOUND)
            {
                fprintf(stderr, "%s not found\n", argv[i]);
                exit_code = EXIT_FAILURE;
            }
            else
            {
                while ((bytes_read = nqp_read(fd, buffer, 256)) > 0)
                {
                    for (ssize_t i = 0; i < bytes_read; i++)
                    {
                        putchar(buffer[i]);
                    }
                }

                nqp_close(fd);
            }
        }
    // Optionally, parent process can wait for the child to finish.
}
