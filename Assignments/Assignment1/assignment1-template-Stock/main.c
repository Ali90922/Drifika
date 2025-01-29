#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nqp_io.h"

// Function prototype declaration
void print_menu(void);

void print_menu(void)
{
    printf("\nCommands:\n");
    printf("  mount <fs_image>  - Mount an exFAT file system\n");
    printf("  open <filename>   - Open a file\n");
    printf("  read <fd> <size>  - Read bytes from an open file\n");
    printf("  getdents <fd> <size> - List directory entries\n");
    printf("  close <fd>        - Close an open file\n");
    printf("  unmount           - Unmount the file system\n");
    printf("  exit              - Exit the program\n");
}

int main(void)
{
    char command[256];
    char arg1[128]; // Removed unused variable `arg2`
    int fd;
    size_t count;
    char buffer[1024]; // For reading file data
    nqp_error status;

    printf("Welcome to exFAT CLI Tester\n");
    print_menu();

    while (1)
    {
        printf("\n> ");
        fgets(command, sizeof(command), stdin);
        command[strcspn(command, "\n")] = 0; // Remove newline

        if (strncmp(command, "mount", 5) == 0)
        {
            if (sscanf(command, "mount %127s", arg1) == 1) // Prevent buffer overflow
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
        else if (strncmp(command, "open", 4) == 0)
        {
            if (sscanf(command, "open %127s", arg1) == 1)
            {
                fd = nqp_open(arg1);
                if (fd >= 0)
                {
                    printf("Opened file '%s', fd=%d\n", arg1, fd);
                }
                else
                {
                    printf("Failed to open file '%s'\n", arg1);
                }
            }
            else
            {
                printf("Invalid command format. Usage: open <filename>\n");
            }
        }
        else if (strncmp(command, "read", 4) == 0)
        {
            if (sscanf(command, "read %d %zu", &fd, &count) == 2)
            {
                ssize_t bytes = nqp_read(fd, buffer, count);
                if (bytes > 0)
                {
                    buffer[bytes] = '\0';
                    printf("Read %zd bytes: %s\n", bytes, buffer);
                }
                else
                {
                    printf("Failed to read from fd=%d\n", fd);
                }
            }
            else
            {
                printf("Invalid command format. Usage: read <fd> <size>\n");
            }
        }
        else if (strncmp(command, "getdents", 8) == 0)
        {
            if (sscanf(command, "getdents %d %zu", &fd, &count) == 2)
            {
                ssize_t bytes = nqp_getdents(fd, buffer, count);
                if (bytes > 0)
                {
                    printf("Read %zd bytes from directory fd=%d\n", bytes, fd);
                }
                else
                {
                    printf("Failed to list directory entries\n");
                }
            }
            else
            {
                printf("Invalid command format. Usage: getdents <fd> <size>\n");
            }
        }
        else if (strncmp(command, "close", 5) == 0)
        {
            if (sscanf(command, "close %d", &fd) == 1)
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
        else if (strncmp(command, "unmount", 7) == 0)
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
        else if (strncmp(command, "exit", 4) == 0)
        {
            printf("Exiting...\n");
            break;
        }
        else
        {
            printf("Unknown command.\n");
            print_menu();
        }
    }
    return 0;
}
