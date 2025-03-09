#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE

#include <fcntl.h> // For mkstemp, memfd_create
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <readline/readline.h>
#include <readline/history.h>

/* The NQP library headers (exfat, etc.) */
#include "nqp_io.h"

#define BUFFER_SIZE 1024
#define MAX_LINE_SIZE 256
#define MAX_ARGS 20
#define MAX_CMDS 20 /* Maximum number of subcommands in a pipeline */

/* Globals */
char cwd[MAX_LINE_SIZE] = "/";
int pipeline_mode = 0;  /* set to 1 when processing a pipeline */
static int log_fd = -1; /* -1 => no logging */

/* We'll need this to inherit the parent's environment for execve. */
extern char **environ;

/* Forward declarations for built-ins */
void handle_cd(char *dir);
void handle_pwd(void);
void handle_ls(void);

/* The main “exec in child” logic (no return). */
void LaunchFunction(char **cmd_argv, char *input_file, int input_fd_override) __attribute__((noreturn));

/* Helpers for input redirection & file-arg substitution. */
int setup_input_redirection(const char *filename);
void fix_file_args(char **cmd_argv);

/* -----------------------------------------------------------------------
 * Logging (duplicate output to both stdout and log file, if open)
 * ----------------------------------------------------------------------- */
void shell_write(const char *str)
{
    write(STDOUT_FILENO, str, strlen(str));
    if (log_fd >= 0)
    {
        write(log_fd, str, strlen(str));
    }
}

void shell_write_buf(const char *buf, ssize_t n)
{
    if (n > 0)
    {
        write(STDOUT_FILENO, buf, n);
        if (log_fd >= 0)
        {
            write(log_fd, buf, n);
        }
    }
}

/* -----------------------------------------------------------------------
 * Built-in: clear the screen
 * ----------------------------------------------------------------------- */
void handle_clear(void)
{
    /* Typical ANSI escape codes to move cursor home and clear screen */
    /* \033[H  => move cursor to top-left
       \033[2J => clear entire screen
    */
    shell_write("\033[H\033[2J");
}

/* ============================================================================
 * Built-in commands
 * ============================================================================
 */
void handle_pwd(void)
{
    char buf[512];
    snprintf(buf, sizeof(buf), "%s\n", cwd);
    shell_write(buf);
}

void handle_cd(char *dir)
{
    if (!dir)
    {
        fprintf(stderr, "cd: missing argument\n");
        return;
    }

    /* Handle "cd .." */
    if (strcmp(dir, "..") == 0)
    {
        /* If already at root, do nothing */
        if (strcmp(cwd, "/") == 0)
            return;

        char *last_slash = strrchr(cwd, '/');
        if (!last_slash)
        {
            /* Safety: if somehow cwd doesn't contain '/', report error */
            fprintf(stderr, "cd: cwd is invalid (missing slash)\n");
            return;
        }

        /* If the only slash is at the start => new cwd is "/" */
        if (last_slash == cwd)
        {
            strcpy(cwd, "/");
        }
        else
        {
            /* Truncate at the last slash */
            *last_slash = '\0';
        }
        return;
    }

    /* Build a path from cwd + dir if dir is not absolute. */
    char path_copy[256];
    if (dir[0] != '/')
    {
        /* Make sure we always end up with a leading slash if we were at "/" */
        if (strcmp(cwd, "/") == 0)
            snprintf(path_copy, sizeof(path_copy), "/%s", dir);
        else
            snprintf(path_copy, sizeof(path_copy), "%s/%s", cwd, dir);
    }
    else
    {
        strncpy(path_copy, dir, sizeof(path_copy));
        path_copy[sizeof(path_copy) - 1] = '\0';
    }

    /* Try opening via NQP to confirm it exists/is valid */
    if (nqp_open(path_copy) != -1)
    {
        strcpy(cwd, path_copy);
    }
    else
    {
        fprintf(stderr, "Directory %s not found\n", path_copy);
    }
}

void handle_ls(void)
{
    nqp_dirent entry = {0};
    int fd = nqp_open(cwd);
    if (fd == NQP_FILE_NOT_FOUND)
    {
        fprintf(stderr, "%s not found\n", cwd);
        return;
    }

    ssize_t dirents_read;
    while ((dirents_read = nqp_getdents(fd, &entry, 1)) > 0)
    {
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

/* ============================================================================
 * fix_file_args: For each argument that is actually an existing nqp file, we
 * copy it out to a /tmp file on the host so the child process can open it.
 * (Used by commands like: `myprogram some_nqp_file`.)
 * ============================================================================
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
            abs_path[sizeof(abs_path) - 1] = '\0';
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
            /* Copy file contents from nqp to tmp. */
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

            /* Replace the argument with the new /tmp filename. */
            cmd_argv[i] = strdup(tmp_template);
        }
    }
}

/* ============================================================================
 * setup_input_redirection: Copy an nqp file into a memfd so we can read it.
 * Then return that memfd FD. Or -1 on error.
 * ============================================================================
 */
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
        input_abs[sizeof(input_abs) - 1] = '\0';
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

/* ============================================================================
 * LaunchFunction: In this child process, sets up input redirection (if any),
 * fixes file args (if not in pipeline for head/tail), then does an exec
 * by copying the NQP file into a memfd, checking if it's #! or ELF, etc.
 *
 * This function never returns (calls _exit on error or after exec).
 * ============================================================================
 */
void LaunchFunction(char **cmd_argv, char *input_file, int input_fd_override)
{
    int exec_fd;
    char abs_path[MAX_LINE_SIZE];

    /* Build absolute path for the command from the current directory. */
    if (strcmp(cwd, "/") == 0)
        snprintf(abs_path, sizeof(abs_path), "%s", cmd_argv[0]);
    else
        snprintf(abs_path, sizeof(abs_path), "%s/%s", cwd, cmd_argv[0]);

    /* If the command starts with "._", skip that part. */
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

    /* Copy the file contents from nqp into the memfd. */
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

    /* If not in pipeline for head/tail, fix file args. */
    if (!(pipeline_mode && (strcmp(cmd_argv[0], "head") == 0 ||
                            strcmp(cmd_argv[0], "tail") == 0)))
    {
        fix_file_args(cmd_argv);
    }

    /* Input redirection if needed. */
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

    /*
     * IMPORTANT: Remove the real chdir(cwd).
     *
     * // if (chdir(cwd) == -1)
     * // {
     * //     perror("chdir");
     * //     _exit(1);
     * // }
     */

    /* If #! script => copy to temp file and execve. Otherwise fexecve. */
    if (debug_header[0] == '#' && debug_header[1] == '!')
    {
        /* Shell script. Copy in-memory file to a /tmp script, then execve. */
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

/* ============================================================================
 * LaunchPipeline: handle multiple subcommands separated by '|'.
 * This supports any number of pipes, not just one.
 * ============================================================================
 */
void LaunchPipeline(char *line)
{
    pipeline_mode = 1;

    /* Split on '|' into up to MAX_CMDS subcommands. */
    char *cmd_strings[MAX_CMDS];
    memset(cmd_strings, 0, sizeof(cmd_strings));

    int num_cmds = 0;
    char *savep;
    char *segment = strtok_r(line, "|", &savep);
    while (segment && num_cmds < MAX_CMDS)
    {
        cmd_strings[num_cmds++] = segment;
        segment = strtok_r(NULL, "|", &savep);
    }
    if (num_cmds < 1)
    {
        fprintf(stderr, "No commands found in pipeline.\n");
        pipeline_mode = 0;
        return;
    }

    /* For each subcommand, parse tokens (look for <). */
    char *cmd_argvs[MAX_CMDS][MAX_ARGS];
    char *input_files[MAX_CMDS];
    memset(cmd_argvs, 0, sizeof(cmd_argvs));
    memset(input_files, 0, sizeof(input_files));

    for (int i = 0; i < num_cmds; i++)
    {
        char *sub_line = cmd_strings[i];
        int token_count = 0;

        char *savep_sub;
        char *tok = strtok_r(sub_line, " ", &savep_sub);
        while (tok && token_count < MAX_ARGS - 1)
        {
            if (strcmp(tok, "<") == 0)
            {
                tok = strtok_r(NULL, " ", &savep_sub);
                if (!tok)
                {
                    fprintf(stderr, "Syntax error: no input file specified\n");
                    pipeline_mode = 0;
                    return;
                }
                input_files[i] = strdup(tok);
            }
            else
            {
                cmd_argvs[i][token_count++] = tok;
            }
            tok = strtok_r(NULL, " ", &savep_sub);
        }
        cmd_argvs[i][token_count] = NULL;

        if (token_count == 0 && !input_files[i])
        {
            fprintf(stderr, "Empty command in pipeline?\n");
            pipeline_mode = 0;
            return;
        }
    }

    /* Create (num_cmds - 1) pipes for the chain. */
    int pipes[MAX_CMDS - 1][2];
    for (int i = 0; i < num_cmds - 1; i++)
    {
        if (pipe(pipes[i]) == -1)
        {
            perror("pipe");
            pipeline_mode = 0;
            return;
        }
    }

    /* Also create final pipe for logging if needed. */
    int final_pipe[2];
    int using_log_pipe = 0;
    if (log_fd >= 0)
    {
        if (pipe(final_pipe) == -1)
        {
            perror("pipe (final logging)");
            pipeline_mode = 0;
            return;
        }
        using_log_pipe = 1;
    }

    pid_t pids[MAX_CMDS];
    for (int i = 0; i < num_cmds; i++)
    {
        pid_t pid = fork();
        if (pid < 0)
        {
            perror("fork");
            pipeline_mode = 0;
            return;
        }
        if (pid == 0)
        {
            /* Child i */
            if (i > 0)
            {
                /* read from pipes[i-1][0] => STDIN */
                dup2(pipes[i - 1][0], STDIN_FILENO);
            }
            if (i < num_cmds - 1)
            {
                /* write to pipes[i][1] => STDOUT */
                dup2(pipes[i][1], STDOUT_FILENO);
            }
            else
            {
                /* last command => if logging, write to final_pipe[1] */
                if (using_log_pipe)
                {
                    dup2(final_pipe[1], STDOUT_FILENO);
                }
            }

            /* close all pipe FDs not needed in child */
            for (int j = 0; j < num_cmds - 1; j++)
            {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            if (using_log_pipe)
            {
                close(final_pipe[0]);
                close(final_pipe[1]);
            }

            /* LaunchFunction never returns. */
            LaunchFunction(cmd_argvs[i], input_files[i], -1);
        }
        else
        {
            /* parent */
            pids[i] = pid;
        }
    }

    /* Parent closes the chain pipes. */
    for (int i = 0; i < num_cmds - 1; i++)
    {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }
    if (using_log_pipe)
    {
        close(final_pipe[1]); /* we read from final_pipe[0] */
    }

    /* Wait for all but last child, or do concurrency.
       We'll just wait in order here. */
    for (int i = 0; i < num_cmds - 1; i++)
    {
        waitpid(pids[i], NULL, 0);
    }

    /* If logging, read from final_pipe[0] => shell_write_buf => logs+stdout. */
    if (using_log_pipe)
    {
        char buf[BUFFER_SIZE];
        ssize_t rcount;
        while ((rcount = read(final_pipe[0], buf, sizeof(buf))) > 0)
        {
            shell_write_buf(buf, rcount);
        }
        close(final_pipe[0]);
    }

    /* Finally wait for the last child. */
    waitpid(pids[num_cmds - 1], NULL, 0);

    pipeline_mode = 0;
}

/* ============================================================================
 * The main shell loop (single command or pipeline).
 * If user runs: ./nqp_shell volume.img -o log.txt, we set log_fd accordingly.
 * ============================================================================
 */
int main_pipe(int argc, char *argv[], char *envp[])
{
    (void)envp; // unused

    /* Check command-line for -o option. */
    if (argc == 4 && strcmp(argv[2], "-o") == 0)
    {
        log_fd = open(argv[3], O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (log_fd < 0)
        {
            perror("open log file");
            exit(EXIT_FAILURE);
        }
    }
    else if (argc != 2)
    {
        fprintf(stderr, "Usage: %s volume.img [-o log.txt]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    /* Mount the NQP volume. */
    nqp_error mount_error = nqp_mount(argv[1], NQP_FS_EXFAT);
    if (mount_error != NQP_OK)
    {
        if (mount_error == NQP_FSCK_FAIL)
            fprintf(stderr, "%s is inconsistent, not mounting.\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    /* Main read/execute loop */
    while (1)
    {
        /* Show prompt */
        char prompt[MAX_LINE_SIZE];
        snprintf(prompt, sizeof(prompt), "%s:\\> ", cwd);
        shell_write(prompt);

        /* Read line with readline (we supply our prompt manually) */
        char *line = readline("");
        if (!line)
        {
            shell_write("\nExiting shell...\n");
            break;
        }
        if (strlen(line) > 0)
            add_history(line);

        /* If empty line, skip */
        if (strlen(line) == 0)
        {
            free(line);
            continue;
        }

        /* Check for pipeline(s). If there's at least one '|', handle them. */
        if (strchr(line, '|'))
        {
            LaunchPipeline(line);
            free(line);
            continue;
        }

        /* Otherwise single command. Possibly with < redirection. */
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

        /* Check built-in commands */
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
        else if (strcmp(tokens[0], "clear") == 0)
        {
            handle_clear();
            free(line);
            continue;
        }

        /* If not a built-in, parse for optional "< file". */
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

        /* If logging => capture child output in pipe so we can tee it to log. */
        if (log_fd >= 0)
        {
            int final_pipe[2];
            if (pipe(final_pipe) == -1)
            {
                perror("pipe");
                free(line);
                continue;
            }
            pid_t pid = fork();
            if (pid < 0)
            {
                perror("fork single cmd");
                close(final_pipe[0]);
                close(final_pipe[1]);
                free(line);
                continue;
            }
            if (pid == 0)
            {
                /* child => write to final_pipe[1], then LaunchFunction. */
                dup2(final_pipe[1], STDOUT_FILENO);
                close(final_pipe[0]);
                close(final_pipe[1]);
                LaunchFunction(cmd_argv, input_file, -1); /* never returns */
            }
            else
            {
                /* parent => read from final_pipe[0] */
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
            /* No logging => child writes directly to stdout. */
            pid_t pid = fork();
            if (pid < 0)
            {
                perror("fork single cmd no-log");
                free(line);
                continue;
            }
            if (pid == 0)
            {
                LaunchFunction(cmd_argv, input_file, -1);
            }
            else
            {
                waitpid(pid, NULL, 0);
            }
        }

        free(line);
    }

    /* Unmount or close log file if needed */
    if (log_fd >= 0)
        close(log_fd);

    return EXIT_SUCCESS;
}

/* Standard main entry point */
int main(int argc, char *argv[], char *envp[])
{
    return main_pipe(argc, argv, envp);
}
