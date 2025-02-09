#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "nqp_io.h" // This header should declare nqp_mount, nqp_open, nqp_read, nqp_getdents, nqp_close, nqp_unmount, etc.

// Function prototypes for functions defined only in this file:
void print_menu(void);
void list_img_files(void);

// If not declared in nqp_io.h, declare the prototype for print_open_file_table() here.
// void print_open_file_table(void);  // (Only needed if you plan to use this command)

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
    size_t dummy_count; // We ignore this value because we'll always read one entry at a time.
    char buffer[1024];  // Not used in getdents branch now.
    nqp_error status;

    printf("Welcome to exFAT CLI Tester\n");
    list_img_files();
    print_menu();

    while (1)
    {
        printf("\n> ");
        if (fgets(command, sizeof(command), stdin) == NULL)
            break;
        command[strcspn(command, "\n")] = '\0';

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
                    printf("Opened file/directory '%s', fd=%d\n", arg1, fd);
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
            if (sscanf(command, "read %d %zu", &fd, &dummy_count) == 2)
            {
                ssize_t bytes = nqp_read(fd, buffer, dummy_count);
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
            if (sscanf(command, "getdents %d %zu", &fd, &dummy_count) == 2)
            {
                // This branch uses the ls style provided by the professor.
                // We read one directory entry at a time.
                ssize_t dirents_read;
                nqp_dirent entry = {0};
                while ((dirents_read = nqp_getdents(fd, &entry, 1)) > 0)
                {
                    // Print the directory entry.
                    printf("%" PRIu64 " %s", entry.inode_number, entry.name);
                    if (entry.type == DT_DIR)
                    {
                        putchar('/');
                    }
                    putchar('\n');

                    // Free the allocated name string.
                    free(entry.name);

                    // Reset the entry structure to zero for the next call.
                    memset(&entry, 0, sizeof(nqp_dirent));
                }

                if (dirents_read == -1)
                {
                    fprintf(stderr, "Error: %s is not a directory\n", arg1);
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
            list_img_files();
        }
    }
    return 0;
}
