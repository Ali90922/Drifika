#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "nqp_io.h" // This header should declare print_open_file_table() among other functions

// Function prototypes for functions defined only in this file:
void print_menu(void);
void list_img_files(void);

// Add the prototype for print_open_file_table if not declared in nqp_io.h
void print_open_file_table(void);

void print_menu(void)
{
    printf("\nCommands:\n");
    printf("  mount <fs_image>        - Mount an exFAT file system\n");
    printf("  open <filename>         - Open a file\n");
    printf("  read <fd> <size>        - Read bytes from an open file\n");
    printf("  getdents <fd> <size>    - List directory entries\n");
    printf("  close <fd>              - Close an open file\n");
    printf("  oft                     - Print the Open File Table (OFT)\n");
    printf("  unmount                 - Unmount the file system\n");
    printf("  exit                    - Exit the program\n");
}

// Function to list .img files without using dirent.h
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
    size_t count;
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

                    // Cast the buffer to an array of nqp_dirent entries.
                    nqp_dirent *entries = (nqp_dirent *)buffer;
                    size_t num_entries = bytes / sizeof(nqp_dirent);

                    // Print each directory entry.
                    for (size_t i = 0; i < num_entries; i++)
                    {
                        printf("Entry %zu:\n", i);
                        printf("  Inode: %" PRIu64 "\n", entries[i].inode_number);
                        printf("  Name: %s\n", entries[i].name);
                        printf("  Type: %s\n", (entries[i].type == DT_DIR) ? "Directory" : "Regular File");
                        printf("  Name Length: %zu\n", entries[i].name_len);
                    }
                }
                else
                {
                    printf("Failed to list directory entries\n");
                    printf("Value: %zd\n", bytes);
                }
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
        else if (strncmp(command, "oft", 3) == 0)
        {
            // Call the function to print the Open File Table
            print_open_file_table();
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
            list_img_files(); // Re-list available .img files for user reference
        }
    }
    return 0;
}
