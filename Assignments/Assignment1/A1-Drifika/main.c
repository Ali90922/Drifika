#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "nqp_io.h" // This header should declare all exFAT API functions and types

// Function prototypes for functions defined only in this file:
void print_menu(void);
void list_img_files(void);

void print_menu(void)
{
    printf("\nCommands:\n");
    printf("  mount <fs_image>        - Mount an exFAT file system\n");
    printf("  open <filename>         - Open a file or directory\n");
    printf("  read <fd>               - Read a file (cat-style, in 256-byte chunks)\n");
    printf("  getdents <fd> <dummy>   - List directory entries (reads one entry at a time)\n");
    printf("  close <fd>              - Close an open file\n");
    printf("  unmount                 - Unmount the file system\n");
    printf("  exit                    - Exit the program\n");
}

void list_img_files(void)
{
    printf("\nAvailable .img files:\n");
    system("ls *.img 2>/dev/null || echo '  No .img files found.'");
}

int main(void)
{
    char command[256];
    char arg1[128];
    int fd;
    size_t dummy; // Dummy variable (used for getdents command)
    char buffer[1024];
    nqp_error status;

    printf("Welcome to exFAT CLI Tester\n");
    list_img_files(); // Show available .img files
    print_menu();

    while (1)
    {
        printf("\n> ");
        if (fgets(command, sizeof(command), stdin) == NULL)
            break;
        command[strcspn(command, "\n")] = '\0'; // Remove newline

        if (strncmp(command, "mount", 5) == 0)
        {
            if (sscanf(command, "mount %127s", arg1) == 1)
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
        else if (strncmp(command, "read", 4) == 0)
        {
            // Cat-style reading: read in 256-byte chunks until EOF.
            if (sscanf(command, "read %d", &fd) == 1)
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
        else if (strncmp(command, "getdents", 8) == 0)
        {
            // For the professor's ls: ignore the dummy count and read one entry at a time.
            if (sscanf(command, "getdents %d %zu", &fd, &dummy) == 2)
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
            list_img_files(); // Re-list available .img files for reference
        }
    }
    return 0;
}
