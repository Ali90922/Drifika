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
#include <readline/readline.h>
#include <readline/history.h>

#define BUFFER_SIZE 1024
#define MAX_LINE_SIZE 256
#define MAX_ARGS 20

/* Global current working directory */
char cwd[MAX_LINE_SIZE] = "/";

/* Global flag: set to 1 when executing a pipeline */
int pipeline_mode = 0;

/* We'll need this to inherit the parent's environment for execve. */
extern char **environ;

/* Global log file descriptor, -1 if not logging. */
static int log_fd = -1;

/* Forward declarations */
void handle_cd(char *dir);
void handle_pwd(void);
void handle_ls(void);

/* We do not fork in LaunchFunction; it *execs* in this process. */
void LaunchFunction(char **cmd_argv, char *input_file, int input_fd_override) __attribute__((noreturn));

void LaunchSinglePipe(char *line);
int setup_input_redirection(const char *filename);
void fix_file_args(char **cmd_argv);

/* ------------------------------------------------------------------
   Helper: if we're logging, also write to log_fd.
   If not, just write to stdout.
   You can do fputs or write directly. Here we do a simple approach.
   ------------------------------------------------------------------ */
void shell_write(const char *str)
{
    /* Always write to stdout: */
    write(STDOUT_FILENO, str, strlen(str));

    /* Also write to log if enabled: */
    if (log_fd >= 0)
    {
        write(log_fd, str, strlen(str));
    }
}

/* This version writes a buffer of arbitrary length (for piping child’s output). */
void shell_write_buf(const char *buf, ssize_t n)
{
    if (n <= 0)
        return;
    /* stdout */
    write(STDOUT_FILENO, buf, n);
    /* log file if open */
    if (log_fd >= 0)
    {
        write(log_fd, buf, n);
    }
}

/* fix_file_args implementation unchanged... */
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

/* setup_input_redirection implementation unchanged... */
int setup_input_redirection(const char *filename)
{
    char input_abs[MAX_LINE_SIZE];
    if (filename[0] != '/')
    {
        if (strcmp(cwd, "/") == 0)
            snprintf(input_abs, sizeof(input_abs), "%s", filename);
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
        nqp_close(fd);
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
            nqp_close(fd);
            return -1;
        }
    }
    if (r < 0)
    {
        fprintf(stderr, "Error reading input file %s\n", input_abs);
        close(memfd_in);
        nqp_close(fd);
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
    /* Print it with shell_write so it goes to log if needed. */
    char buf[512];
    snprintf(buf, sizeof(buf), "%s\n", cwd);
    shell_write(buf);
}

/* handle_cd: Change directory; supports "cd .." */
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
            snprintf(path_copy, sizeof(path_copy), "%s", dir);
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

/* handle_ls: List the contents of the current directory */
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
            /* Let’s write to stdout normally here or with shell_write? */
            char buf[512];
            snprintf(buf, sizeof(buf), "%lu %s", entry.inode_number, entry.name);
            if (entry.type == DT_DIR)
                strncat(buf, "/", sizeof(buf) - strlen(buf) - 1);
            strncat(buf, "\n", sizeof(buf) - strlen(buf) - 1);
            shell_write(buf);

            free(entry.name);
        }
        if (dirents_read == -1)
            fprintf(stderr, "%s is not a directory\n", cwd);
        nqp_close(fd);
    }
}

/* ---------------------------------------------------------------------
   LaunchFunction: This now never returns, it eventually calls exit()
   (like exec) so we can be in the same child that has STDOUT/STDIN set up.

   Because we do a second fork, though, this code is the old logic that
   double-forks. We'll fix that by removing the fork from inside here.

   => We'll make this function a "no return", so it calls _exit in errors
      or does exec/fexecve at the end. The shell must do 1 fork for each
      command it wants to run. This child calls LaunchFunction.
   --------------------------------------------------------------------- */
void LaunchFunction(char **cmd_argv, char *input_file, int input_fd_override)
{
    /* Instead of forking again, we do the logic directly
       and then exec. On error, call _exit(1) or so. */

    int exec_fd = 0;
    char abs_path[MAX_LINE_SIZE];

    if (strcmp(cwd, "/") == 0)
        snprintf(abs_path, sizeof(abs_path), "%s", cmd_argv[0]);
    else
        snprintf(abs_path, sizeof(abs_path), "%s/%s", cwd, cmd_argv[0]);

    /* Special-case: if the command name starts with "._", skip the "._" */
    if (strncmp(cmd_argv[0], "._", 2) == 0)
        cmd_argv[0] += 2;

    exec_fd = nqp_open(abs_path);
    if (exec_fd == NQP_FILE_NOT_FOUND)
    {
        fprintf(stderr, "Command %s not found\n", abs_path);
        _exit(127);
    }
    /* Copy the command into a memfd so we can fexecve or handle #!. */
    int InMemoryFile = memfd_create("In-Memory-File", MFD_CLOEXEC);
    if (InMemoryFile == -1)
    {
        perror("memfd_create");
        _exit(1);
    }

    ssize_t bytes_read, bytes_written;
    char buffer[BUFFER_SIZE];
    while ((bytes_read = nqp_read(exec_fd, buffer, BUFFER_SIZE)) > 0)
    {
        bytes_written = write(InMemoryFile, buffer, bytes_read);
        if (bytes_written != bytes_read)
        {
            fprintf(stderr, "Error writing to in-memory file\n");
            _exit(1);
        }
    }
    nqp_close(exec_fd);

    if (bytes_read < 0)
    {
        fprintf(stderr, "Error reading the source file\n");
        _exit(1);
    }
    if (fchmod(InMemoryFile, 0755) == -1)
    {
        perror("fchmod");
        _exit(1);
    }
    if (lseek(InMemoryFile, 0, SEEK_SET) == -1)
    {
        perror("lseek before header debug");
        _exit(1);
    }

    unsigned char debug_header[16];
    bytes_read = read(InMemoryFile, debug_header, sizeof(debug_header));
    if (bytes_read != (ssize_t)sizeof(debug_header))
    {
        perror("read header");
        _exit(1);
    }
    if (lseek(InMemoryFile, 0, SEEK_SET) == -1)
    {
        perror("lseek after header debug");
        _exit(1);
    }

    /* If not in pipeline for head/tail, fix file args: */
    if (!(pipeline_mode && (strcmp(cmd_argv[0], "head") == 0 ||
                            strcmp(cmd_argv[0], "tail") == 0)))
    {
        fix_file_args(cmd_argv);
    }

    /* Setup input redirection in this child if needed. */
    if (input_file != NULL || input_fd_override != -1)
    {
        int new_stdin_fd;
        if (input_file != NULL)
        {
            new_stdin_fd = setup_input_redirection(input_file);
            if (new_stdin_fd == -1)
                _exit(1);
        }
        else
        {
            new_stdin_fd = input_fd_override;
        }
        if (dup2(new_stdin_fd, STDIN_FILENO) == -1)
        {
            perror("dup2 for input");
            _exit(1);
        }
        close(new_stdin_fd);
    }

    /* Also, chdir to our shell's working directory. */
    if (chdir(cwd) == -1)
    {
        perror("chdir");
        _exit(1);
    }

    /* #! => copy to a temp file, execve that. Otherwise fexecve. */
    if (debug_header[0] == '#' && debug_header[1] == '!')
    {
        char tmp_template[] = "/tmp/scriptXXXXXX";
        int tmp_fd = mkstemp(tmp_template);
        if (tmp_fd == -1)
        {
            perror("mkstemp");
            _exit(1);
        }
        while ((bytes_read = read(InMemoryFile, buffer, BUFFER_SIZE)) > 0)
        {
            if (write(tmp_fd, buffer, bytes_read) != bytes_read)
            {
                perror("write to tmp file");
                close(tmp_fd);
                _exit(1);
            }
        }
        if (bytes_read < 0)
        {
            perror("read from in-memory file");
            close(tmp_fd);
            _exit(1);
        }
        if (fchmod(tmp_fd, 0755) == -1)
        {
            perror("fchmod tmp file");
            close(tmp_fd);
            _exit(1);
        }
        close(tmp_fd);

        execve(tmp_template, cmd_argv, environ);
        perror("execve (script)");
        _exit(1);
    }
    else
    {
        /* ELF or other binary => fexecve directly. */
        if (fexecve(InMemoryFile, cmd_argv, environ) == -1)
        {
            perror("fexecve");
            _exit(1);
        }
    }
    /* not reached */
    _exit(1);
}

/* LaunchSinglePipe:
   Now that we only handle "one pipe," we do:
   - create pipe for left->right
   - if logging is on, we also intercept the right's final output in another pipe
*/
void LaunchSinglePipe(char *line)
{
    char *saveptr;
    char *left_str = strtok_r(line, "|", &saveptr);
    char *right_str = strtok_r(NULL, "|", &saveptr);

    if (!left_str || !right_str)
    {
        fprintf(stderr, "Syntax error: expected two commands separated by a pipe\n");
        return;
    }

    /* Parse left side for optional < redirection */
    char *left_tokens[MAX_ARGS];
    int left_argc = 0;
    char *input_file = NULL;
    char *saveptr_left;
    char *token = strtok_r(left_str, " ", &saveptr_left);
    while (token && left_argc < MAX_ARGS - 1)
    {
        if (strcmp(token, "<") == 0)
        {
            token = strtok_r(NULL, " ", &saveptr_left);
            if (!token)
            {
                fprintf(stderr, "Syntax error: no input file specified\n");
                return;
            }
            input_file = strdup(token);
        }
        else
        {
            left_tokens[left_argc++] = strdup(token);
        }
        token = strtok_r(NULL, " ", &saveptr_left);
    }
    left_tokens[left_argc] = NULL;

    /* Parse right side */
    char *right_tokens[MAX_ARGS];
    int right_argc = 0;
    char *saveptr_right;
    token = strtok_r(right_str, " ", &saveptr_right);
    while (token && right_argc < MAX_ARGS - 1)
    {
        right_tokens[right_argc++] = strdup(token);
        token = strtok_r(NULL, " ", &saveptr_right);
    }
    right_tokens[right_argc] = NULL;

    /* Pipe for left->right */
    int pipe_fd[2];
    if (pipe(pipe_fd) == -1)
    {
        perror("pipe (left->right)");
        return;
    }

    pipeline_mode = 1;

    /* Fork child for left side */
    pid_t pid_left = fork();
    if (pid_left < 0)
    {
        perror("fork (left)");
        close(pipe_fd[0]);
        close(pipe_fd[1]);
        pipeline_mode = 0;
        return;
    }
    if (pid_left == 0)
    {
        /* in left child: write to pipe_fd[1] => STDOUT */
        dup2(pipe_fd[1], STDOUT_FILENO);
        close(pipe_fd[0]);
        close(pipe_fd[1]);
        /* does not return */
        LaunchFunction(left_tokens, input_file, -1);
    }

    /* Next: the right side might be the final command. If log_fd >= 0,
       we set up a second pipe from right->shell so we can intercept. */
    int final_pipe[2];
    int using_log_pipe = 0;
    if (log_fd >= 0)
    {
        if (pipe(final_pipe) == -1)
        {
            perror("pipe (right->shell logging)");
            close(pipe_fd[0]);
            close(pipe_fd[1]);
            waitpid(pid_left, NULL, 0);
            pipeline_mode = 0;
            return;
        }
        using_log_pipe = 1;
    }

    /* Fork child for right side */
    pid_t pid_right = fork();
    if (pid_right < 0)
    {
        perror("fork (right)");
        close(pipe_fd[0]);
        close(pipe_fd[1]);
        if (using_log_pipe)
        {
            close(final_pipe[0]);
            close(final_pipe[1]);
        }
        waitpid(pid_left, NULL, 0);
        pipeline_mode = 0;
        return;
    }
    if (pid_right == 0)
    {
        /* in right child: read from pipe_fd[0] => STDIN */
        dup2(pipe_fd[0], STDIN_FILENO);
        close(pipe_fd[0]);
        close(pipe_fd[1]);

        if (using_log_pipe)
        {
            /* write final output to final_pipe[1] instead of stdout */
            dup2(final_pipe[1], STDOUT_FILENO);
            close(final_pipe[0]);
            close(final_pipe[1]);
        }
        /* does not return */
        LaunchFunction(right_tokens, NULL, -1);
    }

    /* parent */
    close(pipe_fd[0]);
    close(pipe_fd[1]);

    /* We can wait for left first, or do concurrency. Typically we just wait in order. */
    waitpid(pid_left, NULL, 0);

    if (using_log_pipe)
    {
        /* We must read from final_pipe[0] until EOF, writing everything to both stdout & log */
        close(final_pipe[1]); // not used by parent
        char buf[BUFFER_SIZE];
        ssize_t rcount;
        while ((rcount = read(final_pipe[0], buf, sizeof(buf))) > 0)
        {
            shell_write_buf(buf, rcount);
        }
        close(final_pipe[0]);
    }

    waitpid(pid_right, NULL, 0);

    pipeline_mode = 0;
}

/* main_pipe: Main loop for our one-pipe shell using readline */
int main_pipe(int argc, char *argv[], char *envp[])
{
    char *line = NULL;
    (void)envp; // Unused for now

    /* Check if we have the form: ./nqp_shell volume.img -o log.txt */
    if (argc == 4 && strcmp(argv[2], "-o") == 0)
    {
        /* open log file */
        log_fd = open(argv[3], O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (log_fd < 0)
        {
            perror("open log file");
            exit(EXIT_FAILURE);
        }
        /* do_log is implied by (log_fd >= 0) */
    }
    else if (argc != 2)
    {
        fprintf(stderr, "Usage: %s volume.img [-o log.txt]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    nqp_error mount_error = nqp_mount(argv[1], NQP_FS_EXFAT);
    if (mount_error != NQP_OK)
    {
        if (mount_error == NQP_FSCK_FAIL)
            fprintf(stderr, "%s is inconsistent, not mounting.\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        /* Build the prompt; use shell_write so it goes to log if needed. */
        char prompt[MAX_LINE_SIZE];
        snprintf(prompt, sizeof(prompt), "%s:\\> ", cwd);
        shell_write(prompt);

        /* Use readline purely to get user input, ignoring auto prompt prints. */
        line = readline(""); /* pass empty prompt to readline, we do ours manually */
        if (line == NULL)
        {
            shell_write("\nExiting shell...\n");
            break;
        }
        if (strlen(line) > 0)
            add_history(line);
        if (strlen(line) == 0)
        {
            free(line);
            continue;
        }

        /* Check for pipeline */
        if (strchr(line, '|') != NULL)
        {
            LaunchSinglePipe(line);
            free(line);
            continue;
        }

        /* Otherwise single command. Possibly with <. Parse it. */
        char *tokens[MAX_ARGS];
        int token_count = 0;
        char *tok = strtok(line, " ");
        while (tok && token_count < MAX_ARGS - 1)
        {
            tokens[token_count++] = tok;
            tok = strtok(NULL, " ");
        }
        tokens[token_count] = NULL;
        if (token_count == 0)
        {
            free(line);
            continue;
        }

        /* Check for built-in commands */
        if (strcmp(tokens[0], "exit") == 0)
        {
            shell_write("Exiting shell...\n");
            free(line);
            break;
        }
        else if (strcmp(tokens[0], "pwd") == 0)
        {
            handle_pwd();
            free(line);
            continue;
        }
        else if (strcmp(tokens[0], "ls") == 0)
        {
            handle_ls();
            free(line);
            continue;
        }
        else if (strcmp(tokens[0], "cd") == 0)
        {
            handle_cd(tokens[1]);
            free(line);
            continue;
        }

        /* Now parse for < input redirection. */
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
        {
            free(line);
            continue;
        }

        /* If logging is enabled, we need to intercept the child's output in a pipe. */
        if (log_fd >= 0)
        {
            int final_pipe[2];
            if (pipe(final_pipe) == -1)
            {
                perror("pipe (single cmd->shell)");
                free(line);
                continue;
            }
            pid_t pid = fork();
            if (pid < 0)
            {
                perror("fork");
                close(final_pipe[0]);
                close(final_pipe[1]);
                free(line);
                continue;
            }
            if (pid == 0)
            {
                /* child: redirect stdout to final_pipe[1] */
                dup2(final_pipe[1], STDOUT_FILENO);
                close(final_pipe[0]);
                close(final_pipe[1]);
                LaunchFunction(cmd_argv, input_file, -1); /* never returns */
            }
            else
            {
                /* parent: read from final_pipe[0] => shell_write_buf => both stdout/log */
                close(final_pipe[1]);
                char buf[BUFFER_SIZE];
                ssize_t rcount;
                while ((rcount = read(final_pipe[0], buf, sizeof(buf))) > 0)
                {
                    shell_write_buf(buf, rcount);
                }
                close(final_pipe[0]);
                waitpid(pid, NULL, 0);
            }
        }
        else
        {
            /* No logging, simpler: child writes directly to stdout. */
            pid_t pid = fork();
            if (pid < 0)
            {
                perror("fork single cmd");
                free(line);
                continue;
            }
            if (pid == 0)
            {
                /* child => direct stdout */
                LaunchFunction(cmd_argv, input_file, -1);
            }
            else
            {
                waitpid(pid, NULL, 0);
            }
        }

        free(line);
    }

    /* Cleanup */
    if (log_fd >= 0)
        close(log_fd);

    return EXIT_SUCCESS;
}

int main(int argc, char *argv[], char *envp[])
{
    return main_pipe(argc, argv, envp);
}
