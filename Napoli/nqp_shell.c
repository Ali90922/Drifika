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
#include <errno.h>

#define BUFFER_SIZE 1024
#define MAX_LINE_SIZE 256
#define MAX_ARGS 20

// Global variable for current working directory (cwd)
char cwd[MAX_LINE_SIZE] = "/";

/* Function declarations */
void handle_cd(char *dir);
void handle_pwd(void);
void handle_ls(void);
void LaunchFunction(char **cmd_argv, char *input_file);
void LaunchSinglePipe(char *line);
int setup_input_redirection(const char *filename);
void fix_file_args(char **cmd_argv);

/*
 * fix_file_args:
 * For every argument (except the first, which is the command name), check if the file exists on the volume.
 * If so, create a temporary file on the host and copy the file contents into it,
 * then replace the argument with the temporary file’s path.
 */
void fix_file_args(char **cmd_argv)
{
    for (int i = 1; cmd_argv[i] != NULL; i++)
    {
        char abs_path[MAX_LINE_SIZE];
        if (cmd_argv[i][0] != '/')
        {
            if (strcmp(cwd, "/") == 0)
                snprintf(abs_path, sizeof(abs_path), "%s", cmd_argv[i]);
            else
                snprintf(abs_path, sizeof(abs_path), "%s/%s", cwd, cmd_argv[i]);
        }
        else
        {
            strncpy(abs_path, cmd_argv[i], sizeof(abs_path));
        }
        int fd = nqp_open(abs_path);
        if (fd != NQP_FILE_NOT_FOUND)
        {
            char tmp_template[] = "/tmp/volfileXXXXXX";
            int tmp_fd = mkstemp(tmp_template);
            if (tmp_fd == -1)
            {
                perror("mkstemp for file argument");
                close(fd);
                continue;
            }
            ssize_t r, w;
            char buf[BUFFER_SIZE];
            while ((r = nqp_read(fd, buf, BUFFER_SIZE)) > 0)
            {
                w = write(tmp_fd, buf, r);
                if (w != r)
                {
                    perror("write to temporary file for argument");
                    break;
                }
            }
            close(tmp_fd);
            nqp_close(fd);
            cmd_argv[i] = strdup(tmp_template);
        }
    }
}

/*
 * setup_input_redirection:
 * Reads a file from the volume (given its name) into a memory-backed file.
 * Returns a file descriptor for the in-memory file (to be dup2'ed to STDIN) or -1 on error.
 */
int setup_input_redirection(const char *filename)
{
    char input_abs[MAX_LINE_SIZE];
    if (filename[0] != '/')
    {
        if (strcmp(cwd, "/") == 0)
            snprintf(input_abs, sizeof(input_abs), "/%s", filename);
        else
            snprintf(input_abs, sizeof(input_abs), "%s/%s", cwd, filename);
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
    if (lseek(memfd_in, 0, SEEK_SET) == -1)
    {
        perror("lseek on input memfd");
        close(memfd_in);
        return -1;
    }
    return memfd_in;
}

/* handle_pwd: Print the current working directory */
void handle_pwd(void)
{
    printf("%s\n", cwd);
}

/* handle_cd: Change directory. Supports "cd .." */
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
            return;
        char *last_slash = strrchr(cwd, '/');
        if (last_slash == cwd)
            strcpy(cwd, "/");
        else
            *last_slash = '\0';
        return;
    }
    char path_copy[256];
    if (dir[0] != '/')
    {
        if (strcmp(cwd, "/") == 0)
            snprintf(path_copy, sizeof(path_copy), "/%s", dir);
        else
            snprintf(path_copy, sizeof(path_copy), "%s/%s", cwd, dir);
    }
    else
    {
        strncpy(path_copy, dir, sizeof(path_copy));
    }
    if (nqp_open(path_copy) != -1)
        strcpy(cwd, path_copy);
    else
        fprintf(stderr, "Directory %s not found\n", path_copy);
}

/* handle_ls: List directory contents */
void handle_ls(void)
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

/* LaunchFunction: Execute a single command (without a pipe) */
void LaunchFunction(char **cmd_argv, char *input_file)
{
    int exec_fd = 0;
    char abs_path[MAX_LINE_SIZE];

    if (strcmp(cwd, "/") == 0)
        snprintf(abs_path, sizeof(abs_path), "%s", cmd_argv[0]);
    else
        snprintf(abs_path, sizeof(abs_path), "%s/%s", cwd, cmd_argv[0]);

    exec_fd = nqp_open(abs_path);
    if (exec_fd == NQP_FILE_NOT_FOUND)
    {
        fprintf(stderr, "Command %s not found\n", abs_path);
        return;
    }
    printf("File Descriptor for command (%s) is : %d\n", abs_path, exec_fd);

    int InMemoryFile = memfd_create("In-Memory-File", MFD_CLOEXEC);
    printf("File Descriptor for In Memory File is  : %d\n", InMemoryFile);
    if (InMemoryFile == -1)
    {
        perror("memfd_create");
        return;
    }

    ssize_t bytes_read, bytes_written;
    char buffer[BUFFER_SIZE];
    while ((bytes_read = nqp_read(exec_fd, buffer, BUFFER_SIZE)) > 0)
    {
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

    if (fchmod(InMemoryFile, 0755) == -1)
    {
        perror("fchmod");
        return;
    }
    if (lseek(InMemoryFile, 0, SEEK_SET) == -1)
    {
        perror("lseek before header debug");
        return;
    }

    unsigned char debug_header[16];
    ssize_t n = read(InMemoryFile, debug_header, sizeof(debug_header));
    if (n != sizeof(debug_header))
    {
        perror("read header");
        return;
    }
    printf("In-memory file header: ");
    for (size_t i = 0; i < sizeof(debug_header); i++)
        printf("%02x ", debug_header[i]);
    printf("\n");
    fflush(stdout);

    if (lseek(InMemoryFile, 0, SEEK_SET) == -1)
    {
        perror("lseek after header debug");
        return;
    }

    if (debug_header[0] == '#' && debug_header[1] == '!')
    {
        printf("Detected shell script, using temporary file workaround\n");
        fflush(stdout);

        char tmp_template[] = "/tmp/scriptXXXXXX";
        int tmp_fd = mkstemp(tmp_template);
        if (tmp_fd == -1)
        {
            perror("mkstemp");
            exit(1);
        }
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
            if (input_file != NULL)
            {
                int input_fd = setup_input_redirection(input_file);
                if (input_fd == -1)
                    exit(1);
                if (dup2(input_fd, STDIN_FILENO) == -1)
                {
                    perror("dup2 for input");
                    exit(1);
                }
                close(input_fd);
            }
            if (chdir(cwd) == -1)
            {
                perror("chdir");
                exit(1);
            }
            fix_file_args(cmd_argv);
            {
                char *envp[] = {NULL};
                if (execve(tmp_template, cmd_argv, envp) == -1)
                {
                    perror("execve");
                    exit(1);
                }
            }
        }
        else
        {
            int status;
            waitpid(pid, &status, 0);
        }
    }
    else
    {
        pid_t pid = fork();
        if (pid == -1)
        {
            perror("fork");
            return;
        }
        if (pid == 0)
        {
            if (input_file != NULL)
            {
                int input_fd = setup_input_redirection(input_file);
                if (input_fd == -1)
                    exit(1);
                if (dup2(input_fd, STDIN_FILENO) == -1)
                {
                    perror("dup2 for input");
                    exit(1);
                }
                close(input_fd);
            }
            fix_file_args(cmd_argv);
            {
                char *envp[] = {NULL};
                if (fexecve(InMemoryFile, cmd_argv, envp) == -1)
                {
                    perror("fexecve");
                    exit(1);
                }
            }
        }
        else
        {
            int status;
            waitpid(pid, &status, 0);
        }
    }
}

/* LaunchSinglePipe: Supports a single pipe between two commands.
   The input line is assumed to be of the form:
   "left_cmd [args ...] [< infile] | right_cmd [args ...]" */
void LaunchSinglePipe(char *line)
{
    // Split the line into left and right parts.
    char *left_str = strtok(line, "|");
    char *right_str = strtok(NULL, "|");
    if (left_str == NULL || right_str == NULL)
    {
        fprintf(stderr, "Syntax error: expected two commands separated by a pipe\n");
        return;
    }

    // Tokenize left command.
    char *left_tokens[MAX_ARGS];
    int left_argc = 0;
    char *input_file = NULL;
    char *token = strtok(left_str, " ");
    while (token != NULL && left_argc < MAX_ARGS - 1)
    {
        if (strcmp(token, "<") == 0)
        {
            token = strtok(NULL, " ");
            if (token == NULL)
            {
                fprintf(stderr, "Syntax error: no input file specified\n");
                return;
            }
            input_file = token;
        }
        else
        {
            left_tokens[left_argc++] = token;
        }
        token = strtok(NULL, " ");
    }
    left_tokens[left_argc] = NULL;

    // Tokenize right command.
    char *right_tokens[MAX_ARGS];
    int right_argc = 0;
    token = strtok(right_str, " ");
    while (token != NULL && right_argc < MAX_ARGS - 1)
    {
        right_tokens[right_argc++] = token;
        token = strtok(NULL, " ");
    }
    right_tokens[right_argc] = NULL;

    int pipe_fd[2];
    if (pipe(pipe_fd) == -1)
    {
        perror("pipe");
        return;
    }

    pid_t pid1 = fork();
    if (pid1 == 0)
    {
        // Left command: set up output to pipe.
        if (input_file != NULL)
        {
            int in_fd = setup_input_redirection(input_file);
            if (in_fd == -1)
                exit(1);
            dup2(in_fd, STDIN_FILENO);
            close(in_fd);
        }
        dup2(pipe_fd[1], STDOUT_FILENO);
        close(pipe_fd[0]);
        close(pipe_fd[1]);
        LaunchFunction(left_tokens, NULL);
        exit(0);
    }

    pid_t pid2 = fork();
    if (pid2 == 0)
    {
        // Right command: set up input from pipe.
        dup2(pipe_fd[0], STDIN_FILENO);
        close(pipe_fd[0]);
        close(pipe_fd[1]);
        /* For commands like head, remove any file arguments so it reads from STDIN */
        if (strcmp(right_tokens[0], "head") == 0)
        {
            int j = 1;
            for (int i = 1; right_tokens[i] != NULL; i++)
            {
                if (right_tokens[i][0] == '-')
                    right_tokens[j++] = right_tokens[i];
            }
            right_tokens[j] = NULL;
        }
        LaunchFunction(right_tokens, NULL);
        exit(0);
    }

    close(pipe_fd[0]);
    close(pipe_fd[1]);
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
}

/* main_pipe: The shell's main loop for our one-pipe version */
int main_pipe(int argc, char *argv[], char *envp[])
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

    mount_error = nqp_mount(argv[1], NQP_FS_EXFAT);
    if (mount_error != NQP_OK)
    {
        if (mount_error == NQP_FSCK_FAIL)
            fprintf(stderr, "%s is inconsistent, not mounting.\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        printf("%s:\\> ", cwd);
        if (fgets(line_buffer, MAX_LINE_SIZE, stdin) == NULL)
        {
            printf("\nExiting shell...\n");
            break;
        }
        line_buffer[strcspn(line_buffer, "\n")] = '\0';
        if (strlen(line_buffer) == 0)
            continue;

        if (strchr(line_buffer, '|') != NULL)
        {
            LaunchSinglePipe(line_buffer);
            continue;
        }

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
            handle_cd(tokens[1]);
            continue;
        }

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
                    i++;
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

int main(int argc, char *argv[], char *envp[])
{
    return main_pipe(argc, argv, envp);
}
