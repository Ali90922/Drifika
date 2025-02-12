#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "nqp_io.h" // This header should declare all exFAT API functions and types

// Function prototypes for functions defined only in this file:
void print_menu(void);
void cleanup(void);

void print_menu(void)
{
    printf("\nCommands:\n");
    printf("  mount <fs_image>        - Mount an exFAT file system\n");
    printf("  open <filename>         - Open a file or directory\n");
    printf("  read <fd> <size>        - Read bytes from an open file\n");
    printf("  getdents <fd> <dummy>   - List directory entries (reads one entry at a time)\n");
    printf("  close <fd>              - Close an open file\n");
    printf("  unmount                 - Unmount the file system\n");
    printf("  exit                    - Exit the program\n");
}

// A helper function to perform cleanup before exit.
void cleanup(void)
{
    nqp_error status = nqp_unmount();
    if (status == NQP_OK)
    {
        printf("Unmounted successfully during cleanup.\n");
    }
    else
    {
        printf("No file system to unmount or unmount failed during cleanup.\n");
    }
}

int main(void)
{
    // Set stdout to unbuffered to help ensure prompt output on all systems.
    setvbuf(stdout, NULL, _IONBF, 0);

    char command[256];
    char arg1[128];
    int fd;
    size_t dummy; // Dummy variable (used for getdents command)
    char buffer[1024];
    nqp_error status;

    // Print the welcome message and menu only once.
    printf("Welcome to exFAT CLI Tester\n");
    print_menu();

    while (1)
    {
        printf("\n> ");
        if (fgets(command, sizeof(command), stdin) == NULL)
            break;
        command[strcspn(command, "\n")] = '\0'; // Remove newline

        // Create a pointer to traverse the command string.
        char *cmd_ptr = command;
        // Trim leading whitespace.
        while (*cmd_ptr == ' ' || *cmd_ptr == '\t')
            cmd_ptr++;

        if (strncmp(cmd_ptr, "mount", 5) == 0)
        {
            if (sscanf(cmd_ptr, "mount %127s", arg1) == 1)
            {
                status = nqp_mount(arg1, NQP_FS_EXFAT);
                if (status == NQP_OK)
                {
                    printf("Mounted successfully!\n");
                }
                else
                {
                    printf("Mount failed! Error: %d\n", status);
                }
            }
            else
            {
                printf("Invalid command format. Usage: mount <fs_image>\n");
            }
        }
        else if (strncmp(cmd_ptr, "open", 4) == 0)
        {
            if (sscanf(cmd_ptr, "open %127s", arg1) == 1)
            {
                fd = nqp_open(arg1);
                if (fd >= 0)
                {
                    printf("Opened '%s' with fd=%d\n", arg1, fd);
                }
                else
                {
                    printf("Failed to open '%s'\n", arg1);
                }
            }
            else
            {
                printf("Invalid command format. Usage: open <filename>\n");
            }
        }
        else if (strncmp(cmd_ptr, "read", 4) == 0)
        {
            // Cat-style reading: read in 256-byte chunks until EOF.
            if (sscanf(cmd_ptr, "read %d", &fd) == 1)
            {
                ssize_t bytes_read;
                while ((bytes_read = nqp_read(fd, buffer, 256)) > 0)
                {
                    for (ssize_t i = 0; i < bytes_read; i++)
                    {
                        putchar(buffer[i]);
                    }
                }
                putchar('\n');
            }
            else
            {
                printf("Invalid command format. Usage: read <fd>\n");
            }
        }
        else if (strncmp(cmd_ptr, "getdents", 8) == 0)
        {
            // For the professor's ls: ignore the dummy count and read one entry at a time.
            if (sscanf(cmd_ptr, "getdents %d %zu", &fd, &dummy) == 2)
            {
                ssize_t dirents_read;
                nqp_dirent entry = {0};

                // Loop until no more entries are returned.
                while ((dirents_read = nqp_getdents(fd, &entry, 1)) > 0)
                {
                    printf("%" PRIu64 " %s", entry.inode_number, entry.name);
                    if (entry.type == DT_DIR)
                    {
                        putchar('/');
                    }
                    putchar('\n');

                    free(entry.name);
                    memset(&entry, 0, sizeof(nqp_dirent));
                }
                if (dirents_read == -1)
                {
                    fprintf(stderr, "Error: fd %d is not a directory\n", fd);
                }
            }
            else
            {
                printf("Invalid command format. Usage: getdents <fd> <dummy>\n");
            }
        }
        else if (strncmp(cmd_ptr, "close", 5) == 0)
        {
            if (sscanf(cmd_ptr, "close %d", &fd) == 1)
            {
                if (nqp_close(fd) == 0)
                {
                    printf("Closed file descriptor %d\n", fd);
                }
                else
                {
                    printf("Failed to close fd=%d\n", fd);
                }
            }
            else
            {
                printf("Invalid command format. Usage: close <fd>\n");
            }
        }
        else if (strncmp(cmd_ptr, "unmount", 7) == 0)
        {
            status = nqp_unmount();
            if (status == NQP_OK)
            {
                printf("Unmounted successfully!\n");
            }
            else
            {
                printf("Unmount failed!\n");
            }
        }
        else if (strncmp(cmd_ptr, "exit", 4) == 0)
        {
            printf("Exiting...\n");
            cleanup(); // Clean up any resources (e.g., unmount the file system)
            exit(0);
        }
        else
        {
            printf("Unknown command.\n");
            print_menu();
        }
    }
    // In case we exit the loop (e.g., EOF), perform final cleanup.
    cleanup();
    return 0;
}
