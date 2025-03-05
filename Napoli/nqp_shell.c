#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE

#include <fcntl.h> // For mkstemp, memfd_create
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
#define MAX_ARGS 20

// Global variable for current working directory (cwd)
char cwd[MAX_LINE_SIZE] = "/";

/* Function declarations */
void handle_cd(char *dir);
void handle_pwd();
void handle_ls();
void LaunchFunction(char **cmd_argv, char *input_file);

/* Helper: Set up input redirection by reading a file from the volume into a memory file.
   Returns a file descriptor (which should be dup2'ed to STDIN_FILENO) or -1 on error. */
int setup_input_redirection(const char *filename)
{
    char input_abs[MAX_LINE_SIZE];
    // If filename is not absolute, combine with current working directory.
    if (filename[0] != '/')
    {
        if (strcmp(cwd, "/") == 0)
        {
            snprintf(input_abs, sizeof(input_abs), "/%s", filename);
        }
        else
        {
            snprintf(input_abs, sizeof(input_abs), "%s/%s", cwd, filename);
        }
    }
    else
    {
        strncpy(input_abs, filename, sizeof(input_abs));
    }

    int fd = nqp_open(input_abs);
    if (fd == NQP_FILE_NOT_FOUND)
    {
        fprintf(stderr, "Input file %s not found\n", input_abs);
        return -1;
    }
    int memfd_in = memfd_create("In-Memory-Input", MFD_CLOEXEC);
    if (memfd_in == -1)
    {
        perror("memfd_create for input");
        return -1;
    }
    ssize_t r, w;
    char buf[BUFFER_SIZE];
    while ((r = nqp_read(fd, buf, BUFFER_SIZE)) > 0)
    {
        w = write(memfd_in, buf, r);
        if (w != r)
        {
            fprintf(stderr, "Error writing input file to memory file\n");
            close(memfd_in);
            return -1;
        }
    }
    if (r < 0)
    {
        fprintf(stderr, "Error reading input file %s\n", input_abs);
        close(memfd_in);
        return -1;
    }
    nqp_close(fd);
    // Reset offset so that the file is read from the beginning.
    if (lseek(memfd_in, 0, SEEK_SET) == -1)
    {
        perror("lseek on input memfd");
        close(memfd_in);
        return -1;
    }
    return memfd_in;
}

int main(int argc, char *argv[], char *envp[])
{
    char line_buffer[MAX_LINE_SIZE] = {0};
    char *tokens[MAX_ARGS];
    nqp_error mount_error;

    (void)envp; // Unused

    if (argc != 2)
    {
        fprintf(stderr, "Usage: ./nqp_shell volume.img\n");
        exit(EXIT_FAILURE);
    }

    // Mount exFAT filesystem
    mount_error = nqp_mount(argv[1], NQP_FS_EXFAT);
    if (mount_error != NQP_OK)
    {
        if (mount_error == NQP_FSCK_FAIL)
        {
            fprintf(stderr, "%s is inconsistent, not mounting.\n", argv[1]);
        }
        exit(EXIT_FAILURE);
    }

    // Shell loop
    while (1)
    {
        printf("%s:\\> ", cwd);
        if (fgets(line_buffer, MAX_LINE_SIZE, stdin) == NULL)
        {
            printf("\nExiting shell...\n");
            break;
        }
        line_buffer[strcspn(line_buffer, "\n")] = '\0';

        // Tokenize input line (simple whitespace splitting)
        int token_count = 0;
        char *token = strtok(line_buffer, " ");
        while (token != NULL && token_count < MAX_ARGS - 1)
        {
            tokens[token_count++] = token;
            token = strtok(NULL, " ");
        }
        tokens[token_count] = NULL;
        if (token_count == 0)
            continue;

        /* Handle built-in commands first */
        if (strcmp(tokens[0], "exit") == 0)
        {
            printf("Exiting shell...\n");
            break;
        }
        else if (strcmp(tokens[0], "pwd") == 0)
        {
            handle_pwd();
            continue;
        }
        else if (strcmp(tokens[0], "ls") == 0)
        {
            handle_ls();
            continue;
        }
        else if (strcmp(tokens[0], "cd") == 0)
        {
            // Pass the directory argument (e.g., "cd .." or "cd somedir")
            handle_cd(tokens[1]);
            continue;
        }

        /* Process input redirection:
           Look for a "<" token. If found, the next token is the file to use for input.
           Remove these tokens from the command's argument vector. */
        char *cmd_argv[MAX_ARGS];
        int cmd_argc = 0;
        char *input_file = NULL;
        for (int i = 0; i < token_count; i++)
        {
            if (strcmp(tokens[i], "<") == 0)
            {
                if (i + 1 < token_count)
                {
                    input_file = tokens[i + 1];
                    i++; // Skip the filename token.
                }
                else
                {
                    fprintf(stderr, "Syntax error: no input file specified\n");
                    cmd_argc = 0;
                    break;
                }
            }
            else
            {
                cmd_argv[cmd_argc++] = tokens[i];
            }
        }
        cmd_argv[cmd_argc] = NULL;
        if (cmd_argc == 0)
            continue;

        LaunchFunction(cmd_argv, input_file);
    }

    return EXIT_SUCCESS;
}

/* Built-in: Print the current working directory */
void handle_pwd()
{
    printf("%s\n", cwd);
}

/* Built-in: Change directory.
   This version supports "cd .." to go back a directory.
   For any other directory, it attempts to open the given path on the mounted volume. */
void handle_cd(char *dir)
{
    if (dir == NULL)
    {
        fprintf(stderr, "cd: missing argument\n");
        return;
    }
    if (strcmp(dir, "..") == 0)
    {
        if (strcmp(cwd, "/") == 0)
        {
            // Already at root.
            return;
        }
        // Find the last slash and truncate.
        char *last_slash = strrchr(cwd, '/');
        if (last_slash == cwd)
        {
            strcpy(cwd, "/");
        }
        else
        {
            *last_slash = '\0';
        }
        return;
    }
    // For other directories, build the absolute path.
    char path_copy[256];
    if (dir[0] != '/')
    {
        // Relative path: combine current cwd with dir.
        if (strcmp(cwd, "/") == 0)
        {
            snprintf(path_copy, sizeof(path_copy), "/%s", dir);
        }
        else
        {
            snprintf(path_copy, sizeof(path_copy), "%s/%s", cwd, dir);
        }
    }
    else
    {
        // Absolute path.
        strncpy(path_copy, dir, sizeof(path_copy));
    }
    if (nqp_open(path_copy) != -1)
    {
        strcpy(cwd, path_copy);
    }
    else
    {
        fprintf(stderr, "Directory %s not found\n", path_copy);
    }
}

/* Built-in: List directory contents */
void handle_ls()
{
    nqp_dirent entry = {0};
    int fd;
    ssize_t dirents_read;
    fd = nqp_open(cwd);
    if (fd == NQP_FILE_NOT_FOUND)
    {
        fprintf(stderr, "%s not found\n", cwd);
    }
    else
    {
        while ((dirents_read = nqp_getdents(fd, &entry, 1)) > 0)
        {
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

/* LaunchFunction: Loads the executable from the volume into a memory-backed file,
   sets up input redirection if requested, and then forks and executes the command.
   It also handles the shell-script case by using a temporary file workaround. */
void LaunchFunction(char **cmd_argv, char *input_file)
{
    int exec_fd = 0;
    char abs_path[MAX_LINE_SIZE];

    // Build the absolute path for the command executable.
    if (strcmp(cwd, "/") == 0)
    {
        if (cmd_argv[0][0] != '/')
        {
            snprintf(abs_path, sizeof(abs_path), "/%s", cmd_argv[0]);
        }
        else
        {
            strncpy(abs_path, cmd_argv[0], sizeof(abs_path));
        }
    }
    else
    {
        snprintf(abs_path, sizeof(abs_path), "%s/%s", cwd, cmd_argv[0]);
    }

    exec_fd = nqp_open(abs_path);
    if (exec_fd == NQP_FILE_NOT_FOUND)
    {
        fprintf(stderr, "Command %s not found\n", abs_path);
        return;
    }
    printf("File Descriptor for command (%s) is : %d\n", abs_path, exec_fd);

    // Create an in-memory file descriptor for the executable.
    int InMemoryFile = memfd_create("In-Memory-File", MFD_CLOEXEC);
    printf("File Descriptor for In Memory File is  : %d\n", InMemoryFile);
    if (InMemoryFile == -1)
    {
        perror("memfd_create");
        return;
    }

    // Copy executable data from the source file into the in-memory file.
    ssize_t bytes_read, bytes_written;
    char buffer[BUFFER_SIZE];
    size_t total_bytes = 0;
    while ((bytes_read = nqp_read(exec_fd, buffer, BUFFER_SIZE)) > 0)
    {
        total_bytes += bytes_read;
        bytes_written = write(InMemoryFile, buffer, bytes_read);
        if (bytes_written != bytes_read)
        {
            fprintf(stderr, "Error writing to in-memory file\n");
            return;
        }
    }
    if (bytes_read < 0)
    {
        fprintf(stderr, "Error reading the source file\n");
        return;
    }
    printf("Total bytes read from source: %zu\n", total_bytes);

    // Set execute permissions on the in-memory file.
    if (fchmod(InMemoryFile, 0755) == -1)
    {
        perror("fchmod");
        return;
    }

    // Reset the in-memory file offset before debugging.
    if (lseek(InMemoryFile, 0, SEEK_SET) == -1)
    {
        perror("lseek before header debug");
        return;
    }

    // Debug: Read and print the first 16 bytes.
    unsigned char debug_header[16];
    ssize_t n = read(InMemoryFile, debug_header, sizeof(debug_header));
    if (n != sizeof(debug_header))
    {
        perror("read header");
        return;
    }
    printf("In-memory file header: ");
    for (size_t i = 0; i < sizeof(debug_header); i++)
    {
        printf("%02x ", debug_header[i]);
    }
    printf("\n");
    fflush(stdout);

    // Reset offset again before execution.
    if (lseek(InMemoryFile, 0, SEEK_SET) == -1)
    {
        perror("lseek after header debug");
        return;
    }

    /* Fork and execute the command.
       If the file starts with "#!" we assume it is a shell script and use a temporary file workaround.
       In both cases, if input redirection is requested, we set it up in the child process before exec. */
    if (debug_header[0] == '#' && debug_header[1] == '!')
    {
        printf("Detected shell script, using temporary file workaround\n");
        fflush(stdout);

        // Create a temporary file that will remain on disk.
        char tmp_template[] = "/tmp/scriptXXXXXX";
        int tmp_fd = mkstemp(tmp_template);
        if (tmp_fd == -1)
        {
            perror("mkstemp");
            exit(1);
        }
        // Do NOT unlink the temporary file; we need its name for execve.

        // Copy the entire content from the in-memory file to the temporary file.
        if (lseek(InMemoryFile, 0, SEEK_SET) == -1)
        {
            perror("lseek before copying to tmp");
            exit(1);
        }
        while ((bytes_read = read(InMemoryFile, buffer, BUFFER_SIZE)) > 0)
        {
            if (write(tmp_fd, buffer, bytes_read) != bytes_read)
            {
                perror("write to tmp file");
                exit(1);
            }
        }
        if (bytes_read < 0)
        {
            perror("read from in-memory file");
            exit(1);
        }
        // Set execute permissions on the temporary file.
        if (fchmod(tmp_fd, 0755) == -1)
        {
            perror("fchmod tmp file");
            exit(1);
        }
        close(tmp_fd);

        pid_t pid = fork();
        if (pid == -1)
        {
            perror("fork");
            exit(1);
        }
        if (pid == 0)
        {
            // Child process: set up input redirection if specified.
            if (input_file != NULL)
            {
                int input_fd = setup_input_redirection(input_file);
                if (input_fd == -1)
                {
                    exit(1);
                }
                if (dup2(input_fd, STDIN_FILENO) == -1)
                {
                    perror("dup2 for input");
                    exit(1);
                }
                close(input_fd);
            }
            // Change the working directory so relative paths work.
            if (chdir(cwd) == -1)
            {
                perror("chdir");
                exit(1);
            }
            // Replace the first argument with the temporary file name.
            cmd_argv[0] = tmp_template;
            if (execve(tmp_template, cmd_argv, NULL) == -1)
            {
                perror("execve");
                exit(1);
            }
        }
        else
        {
            int status;
            waitpid(pid, &status, 0);
            // Optionally, you can remove the temporary file here.
            // unlink(tmp_template);
        }
    }
    else
    {
        // Otherwise, assume it's a proper ELF binary.
        pid_t pid = fork();
        if (pid == -1)
        {
            perror("fork");
            return;
        }
        if (pid == 0)
        {
            // Child process: set up input redirection if specified.
            if (input_file != NULL)
            {
                int input_fd = setup_input_redirection(input_file);
                if (input_fd == -1)
                {
                    exit(1);
                }
                if (dup2(input_fd, STDIN_FILENO) == -1)
                {
                    perror("dup2 for input");
                    exit(1);
                }
                close(input_fd);
            }
            if (fexecve(InMemoryFile, cmd_argv, NULL) == -1)
            {
                perror("fexecve");
                exit(1);
            }
        }
        else
        {
            int status;
            waitpid(pid, &status, 0);
        }
    }
}
