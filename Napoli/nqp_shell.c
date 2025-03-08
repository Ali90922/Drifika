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

/* We want a maximum buffer size for reading from volume */
#define BUFFER_SIZE 1024
/* Maximum line size for user input, or path building, etc. */
#define MAX_LINE_SIZE 256
#define MAX_ARGS 20

/* Global current working directory on the volume */
char cwd[MAX_LINE_SIZE] = "/";

/* We'll need this to inherit the parent's environment for execve/fexecve. */
extern char **environ;

/* Built‐in command prototypes */
void handle_cd(char *dir);
void handle_pwd(void);
void handle_ls(void);

/* The primary "launch" function for external commands */
void LaunchFunction(char **cmd_argv, char *input_file);

/* If user typed `< file`, set up memfd for input. */
int setup_input_redirection(const char *filename);

/* ========== Built-in commands ========== */

/* handle_pwd: Print the current working directory on the volume */
void handle_pwd(void)
{
    printf("%s\n", cwd);
}

/* handle_cd: Change directory within the mounted volume (e.g., "cd subdir") */
void handle_cd(char *dir)
{
    if (dir == NULL)
    {
        fprintf(stderr, "cd: missing argument\n");
        return;
    }
    /* Support "cd .." to go up one directory */
    if (strcmp(dir, "..") == 0)
    {
        /* If already at root "/", do nothing */
        if (strcmp(cwd, "/") == 0)
            return;
        char *last_slash = strrchr(cwd, '/');
        if (last_slash == cwd)
            strcpy(cwd, "/");
        else
            *last_slash = '\0';
        return;
    }
    /* Construct the absolute path inside the volume */
    char path_copy[MAX_LINE_SIZE];
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
    /* Check if that directory exists on the volume */
    if (nqp_open(path_copy) != -1)
        strcpy(cwd, path_copy);
    else
        fprintf(stderr, "Directory %s not found\n", path_copy);
}

/* handle_ls: List the contents of the current directory on the volume */
void handle_ls(void)
{
    nqp_dirent entry = {0};
    int fd;
    ssize_t dirents_read;
    fd = nqp_open(cwd);
    if (fd == NQP_FILE_NOT_FOUND)
    {
        fprintf(stderr, "%s not found\n", cwd);
        return;
    }
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

/* ========== Input Redirection Helper ========== */

/* If user typed "< somefile", copy that volume file into an anonymous memfd. */
int setup_input_redirection(const char *filename)
{
    char input_abs[MAX_LINE_SIZE];
    /* Build absolute path: "cwd/filename" or just "/filename" */
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
    /* Try opening that file in the volume. */
    int fd = nqp_open(input_abs);
    if (fd == NQP_FILE_NOT_FOUND)
    {
        fprintf(stderr, "Input file %s not found\n", input_abs);
        return -1;
    }
    /* Create an in-memory file for the input data. */
    int memfd_in = memfd_create("In-Memory-Input", MFD_CLOEXEC);
    if (memfd_in == -1)
    {
        perror("memfd_create for input");
        nqp_close(fd);
        return -1;
    }
    /* Copy the volume file's contents into the memfd. */
    ssize_t r, w;
    char buf[BUFFER_SIZE];
    while ((r = nqp_read(fd, buf, BUFFER_SIZE)) > 0)
    {
        w = write(memfd_in, buf, r);
        if (w != r)
        {
            fprintf(stderr, "Error writing input file to memfd\n");
            close(memfd_in);
            nqp_close(fd);
            return -1;
        }
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

/* ========== Launch External Command ========== */

/*
 * LaunchFunction:
 * 1. Construct absolute path from cwd + cmd_argv[0].
 * 2. Open that file via nqp_open.
 * 3. Copy it into a memfd using nqp_read + write.
 * 4. fork() → in child → possibly set up STDIN if input_file != NULL → fexecve().
 */
void LaunchFunction(char **cmd_argv, char *input_file)
{
    /* Build absolute path to the command in the volume. */
    char abs_path[MAX_LINE_SIZE];
    if (strcmp(cwd, "/") == 0)
        snprintf(abs_path, sizeof(abs_path), "%s", cmd_argv[0]);
    else
        snprintf(abs_path, sizeof(abs_path), "%s/%s", cwd, cmd_argv[0]);

    /* Try opening that file in the volume. */
    int exec_fd = nqp_open(abs_path);
    if (exec_fd == NQP_FILE_NOT_FOUND)
    {
        fprintf(stderr, "Command %s not found\n", abs_path);
        return;
    }
    printf("File Descriptor for command (%s) is: %d\n", abs_path, exec_fd);

    /* Create an in-memory file for the command contents. */
    int InMemoryFile = memfd_create("In-Memory-File", MFD_CLOEXEC);
    printf("File Descriptor for InMemoryFile: %d\n", InMemoryFile);
    if (InMemoryFile == -1)
    {
        perror("memfd_create");
        nqp_close(exec_fd);
        return;
    }

    /* Copy from volume into memfd. */
    ssize_t bytes_read, bytes_written;
    char buffer[BUFFER_SIZE];
    while ((bytes_read = nqp_read(exec_fd, buffer, BUFFER_SIZE)) > 0)
    {
        bytes_written = write(InMemoryFile, buffer, bytes_read);
        if (bytes_written != bytes_read)
        {
            fprintf(stderr, "Error writing to in-memory file\n");
            close(InMemoryFile);
            nqp_close(exec_fd);
            return;
        }
    }
    nqp_close(exec_fd);

    if (lseek(InMemoryFile, 0, SEEK_SET) == -1)
    {
        perror("lseek before check header");
        close(InMemoryFile);
        return;
    }

    /* Make the file executable. */
    if (fchmod(InMemoryFile, 0755) == -1)
    {
        perror("fchmod InMemoryFile");
        close(InMemoryFile);
        return;
    }

    /* Check if it starts with "#!" → might be a shell script. */
    unsigned char debug_header[16];
    bytes_read = read(InMemoryFile, debug_header, sizeof(debug_header));
    if (bytes_read < 2)
    {
        fprintf(stderr, "Command file is too short or read error.\n");
        close(InMemoryFile);
        return;
    }
    /* Rewind. */
    if (lseek(InMemoryFile, 0, SEEK_SET) == -1)
    {
        perror("lseek after check header");
        close(InMemoryFile);
        return;
    }

    /* fork → child does fexecve or the script workaround. */
    pid_t pid = fork();
    if (pid == -1)
    {
        perror("fork");
        close(InMemoryFile);
        return;
    }
    if (pid == 0)
    {
        /* Child: if there's "< file", set up STDIN. */
        if (input_file != NULL)
        {
            int redir_fd = setup_input_redirection(input_file);
            if (redir_fd == -1)
                exit(1); /* Already printed error. */

            if (dup2(redir_fd, STDIN_FILENO) == -1)
            {
                perror("dup2 for input");
                close(redir_fd);
                exit(1);
            }
            close(redir_fd);
        }
        /* Also chdir to the "cwd" inside the volume. (So commands that do relative file opens work.) */
        if (chdir(cwd) == -1)
        {
            perror("chdir");
            exit(1);
        }

        /* If it is a #! script, do a "temp file" workaround. */
        if (debug_header[0] == '#' && debug_header[1] == '!')
        {
            printf("Detected shell script, using temporary file workaround\n");
            fflush(stdout);

            /* Create a real temp file. */
            char tmp_template[] = "/tmp/scriptXXXXXX";
            int tmp_fd = mkstemp(tmp_template);
            if (tmp_fd == -1)
            {
                perror("mkstemp");
                exit(1);
            }
            /* Copy from memfd into that temp file. */
            while ((bytes_read = read(InMemoryFile, buffer, BUFFER_SIZE)) > 0)
            {
                if (write(tmp_fd, buffer, bytes_read) != bytes_read)
                {
                    perror("write to tmp file");
                    close(tmp_fd);
                    exit(1);
                }
            }
            if (bytes_read < 0)
            {
                perror("read from InMemoryFile");
                close(tmp_fd);
                exit(1);
            }
            if (fchmod(tmp_fd, 0755) == -1)
            {
                perror("fchmod tmp file");
                close(tmp_fd);
                exit(1);
            }
            close(tmp_fd);

            /* execve that temp file with parent's environment. */
            if (execve(tmp_template, cmd_argv, environ) == -1)
            {
                perror("execve for #! script");
                exit(1);
            }
        }
        else
        {
            /* It's presumably an ELF or other binary, so we do fexecve. */
            if (fexecve(InMemoryFile, cmd_argv, environ) == -1)
            {
                perror("fexecve");
                exit(1);
            }
        }
        /* never returns on success */
    }
    else
    {
        /* Parent: just wait for child. */
        int status;
        waitpid(pid, &status, 0);
        close(InMemoryFile);
    }
}

/* ========== Main Loop ========== */

int main(int argc, char *argv[], char *envp[])
{
    (void)envp;

    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s volume.img\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    /* Attempt to mount the nqp_exfat volume. */
    nqp_error mount_error = nqp_mount(argv[1], NQP_FS_EXFAT);
    if (mount_error != NQP_OK)
    {
        if (mount_error == NQP_FSCK_FAIL)
            fprintf(stderr, "%s is inconsistent, not mounting.\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    /* Our simple shell loop uses readline for user input. */
    while (1)
    {
        char prompt[MAX_LINE_SIZE];
        snprintf(prompt, sizeof(prompt), "%s:\\> ", cwd);

        char *line = readline(prompt);
        if (!line) /* NULL => user pressed Ctrl+D or similar. */
        {
            printf("\nExiting shell...\n");
            break;
        }
        if (strlen(line) > 0)
            add_history(line);

        /* Tokenize the line. */
        char *tokens[MAX_ARGS];
        int token_count = 0;
        char *saveptr = NULL;
        char *token = strtok_r(line, " ", &saveptr);
        while (token && token_count < MAX_ARGS - 1)
        {
            tokens[token_count++] = token;
            token = strtok_r(NULL, " ", &saveptr);
        }
        tokens[token_count] = NULL;

        if (token_count == 0)
        {
            free(line);
            continue;
        }

        /* Check built‐ins: exit, pwd, ls, cd. */
        if (strcmp(tokens[0], "exit") == 0)
        {
            printf("Exiting shell...\n");
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

        /* Possibly parse input redirection: "cmd < inputfile". */
        /* We'll separate that out from the main command. */
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
                    cmd_argc = 0; /* discard the command */
                    break;
                }
            }
            else
            {
                cmd_argv[cmd_argc++] = tokens[i];
            }
        }
        cmd_argv[cmd_argc] = NULL;

        if (cmd_argc > 0)
        {
            /* Launch the external command from the volume. */
            LaunchFunction(cmd_argv, input_file);
        }
        free(line);
    }

    return EXIT_SUCCESS;
}
