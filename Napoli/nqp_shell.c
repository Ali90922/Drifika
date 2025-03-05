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
#define MAX_PIPE_CMDS 10 // Maximum number of piped commands per line

// Global variable for current working directory (cwd)
char cwd[MAX_LINE_SIZE] = "/";

/* Function declarations */
void handle_cd(char *dir);
void handle_pwd();
void handle_ls();
void LaunchFunction(char **cmd_argv, char *input_file);
void LaunchPipeline(char *line);
void LaunchPipelineCommand(char **cmd_argv);
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
            fprintf(stderr, "%s is inconsistent, not mounting.\n", argv[1]);
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
        if (strlen(line_buffer) == 0)
            continue;

        // Check for pipe character.
        if (strchr(line_buffer, '|') != NULL)
        {
            LaunchPipeline(line_buffer);
            continue;
        }

        // Otherwise, process a single command.
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

        /* Handle built-in commands */
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

        /* Process input redirection for a single command */
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
                    i++; // Skip filename token.
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

/* handle_pwd: Print the current working directory */
void handle_pwd()
{
    printf("%s\n", cwd);
}

/* handle_cd: Change directory. Supports "cd .." to go back a directory. */
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

/* handle_ls: List directory contents.
   (No additional filtering is applied here.) */
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

/* LaunchFunction: For a single command (without pipes) */
void LaunchFunction(char **cmd_argv, char *input_file)
{
    int exec_fd = 0;
    char abs_path[MAX_LINE_SIZE];

    // Build absolute path: if cwd is "/" then use the command as is.
    if (strcmp(cwd, "/") == 0)
    {
        snprintf(abs_path, sizeof(abs_path), "%s", cmd_argv[0]);
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

    /* Fork and execute the command.
       Set up input redirection if needed.
       Before exec, fix file arguments so that file names (like Juve.txt) are replaced with temporary files,
       so that the command can open them on the host system. */
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
            cmd_argv[0] = tmp_template;
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

/* LaunchPipeline: Processes a line containing one or more piped commands.
   Only the first command can use input redirection via "<". */
void LaunchPipeline(char *line)
{
    char *segments[MAX_PIPE_CMDS];
    int num_cmds = 0;

    // Split the input line on the pipe '|' character.
    char *segment = strtok(line, "|");
    while (segment != NULL && num_cmds < MAX_PIPE_CMDS)
    {
        segments[num_cmds++] = segment;
        segment = strtok(NULL, "|");
    }

    // For each segment, tokenize it into arguments.
    struct Command
    {
        char *args[MAX_ARGS];
        char *input_file; // Only valid for the first command.
    } commands[MAX_PIPE_CMDS];

    for (int i = 0; i < num_cmds; i++)
    {
        int arg_count = 0;
        commands[i].input_file = NULL;
        char *tok = strtok(segments[i], " ");
        while (tok != NULL && arg_count < MAX_ARGS - 1)
        {
            // Only the first command may have input redirection.
            if (i == 0 && strcmp(tok, "<") == 0)
            {
                tok = strtok(NULL, " ");
                if (tok == NULL)
                {
                    fprintf(stderr, "Syntax error: no input file specified\n");
                    return;
                }
                commands[i].input_file = tok;
            }
            else
            {
                commands[i].args[arg_count++] = tok;
            }
            tok = strtok(NULL, " ");
        }
        commands[i].args[arg_count] = NULL;
    }

    int prev_pipe_fd = -1;
    for (int i = 0; i < num_cmds; i++)
    {
        int pipe_fd[2] = {-1, -1};
        if (i < num_cmds - 1)
        {
            if (pipe(pipe_fd) == -1)
            {
                perror("pipe");
                return;
            }
        }
        pid_t pid = fork();
        if (pid == -1)
        {
            perror("fork");
            return;
        }
        if (pid == 0)
        {
            // Child process:
            if (prev_pipe_fd != -1)
            {
                if (dup2(prev_pipe_fd, STDIN_FILENO) == -1)
                {
                    perror("dup2 prev_pipe_fd");
                    exit(1);
                }
                close(prev_pipe_fd);
            }
            if (i < num_cmds - 1)
            {
                if (dup2(pipe_fd[1], STDOUT_FILENO) == -1)
                {
                    perror("dup2 pipe_fd[1]");
                    exit(1);
                }
                close(pipe_fd[0]);
                close(pipe_fd[1]);
            }
            if (i == 0 && commands[i].input_file != NULL)
            {
                int input_fd = setup_input_redirection(commands[i].input_file);
                if (input_fd == -1)
                    exit(1);
                if (dup2(input_fd, STDIN_FILENO) == -1)
                {
                    perror("dup2 for input redirection");
                    exit(1);
                }
                close(input_fd);
            }
            LaunchPipelineCommand(commands[i].args);
            exit(1);
        }
        else
        {
            if (prev_pipe_fd != -1)
                close(prev_pipe_fd);
            if (i < num_cmds - 1)
            {
                close(pipe_fd[1]);
                prev_pipe_fd = pipe_fd[0];
            }
        }
    }
    for (int i = 0; i < num_cmds; i++)
    {
        int status;
        wait(&status);
    }
}

/* LaunchPipelineCommand: Similar to LaunchFunction but for commands in a pipeline.
   It loads the executable from the volume into a memory-backed file and then execs it. */
void LaunchPipelineCommand(char **cmd_argv)
{
    int exec_fd = 0;
    char abs_path[MAX_LINE_SIZE];

    if (strcmp(cwd, "/") == 0)
    {
        snprintf(abs_path, sizeof(abs_path), "%s", cmd_argv[0]);
    }
    else
    {
        snprintf(abs_path, sizeof(abs_path), "%s/%s", cwd, cmd_argv[0]);
    }

    exec_fd = nqp_open(abs_path);
    if (exec_fd == NQP_FILE_NOT_FOUND)
    {
        fprintf(stderr, "Command %s not found\n", abs_path);
        exit(1);
    }

    int InMemoryFile = memfd_create("In-Memory-File", MFD_CLOEXEC);
    if (InMemoryFile == -1)
    {
        perror("memfd_create");
        exit(1);
    }

    ssize_t bytes_read, bytes_written;
    char buffer[BUFFER_SIZE];
    while ((bytes_read = nqp_read(exec_fd, buffer, BUFFER_SIZE)) > 0)
    {
        bytes_written = write(InMemoryFile, buffer, bytes_read);
        if (bytes_written != bytes_read)
        {
            fprintf(stderr, "Error writing to in-memory file\n");
            exit(1);
        }
    }
    if (bytes_read < 0)
    {
        fprintf(stderr, "Error reading the source file\n");
        exit(1);
    }

    if (fchmod(InMemoryFile, 0755) == -1)
    {
        perror("fchmod");
        exit(1);
    }
    if (lseek(InMemoryFile, 0, SEEK_SET) == -1)
    {
        perror("lseek before header debug");
        exit(1);
    }

    unsigned char debug_header[16];
    ssize_t n = read(InMemoryFile, debug_header, sizeof(debug_header));
    if (n != sizeof(debug_header))
    {
        perror("read header");
        exit(1);
    }

    int is_shell_script = (debug_header[0] == '#' && debug_header[1] == '!');
    if (!is_shell_script)
    {
        fix_file_args(cmd_argv);
    }

    if (is_shell_script)
    {
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
        cmd_argv[0] = tmp_template;
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
        {
            char *envp[] = {NULL};
            if (fexecve(InMemoryFile, cmd_argv, envp) == -1)
            {
                perror("fexecve");
                exit(1);
            }
        }
    }
}

/* setup_input_redirection: Reads a file from the volume into a memory-backed file,
   returns a file descriptor for that memory file, which can be dup2'ed to STDIN.
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
