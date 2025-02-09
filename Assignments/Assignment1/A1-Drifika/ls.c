#include "nqp_io.h"
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>

#define MAX_ENTRIES 128 // Maximum number of directory entries to read at one time

int main(int argc, char **argv)
{
    nqp_error err;
    int fd;

    // Check for correct usage.
    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s <filesystem_image> <directory_path>\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Mount the filesystem image.
    err = nqp_mount(argv[1], NQP_FS_EXFAT);
    if (err != NQP_OK)
    {
        fprintf(stderr, "Failed to mount filesystem image: %s\n", argv[1]);
        return EXIT_FAILURE;
    }

    // Open the directory.
    fd = nqp_open(argv[2]);
    if (fd == NQP_FILE_NOT_FOUND)
    {
        fprintf(stderr, "%s not found\n", argv[2]);
        nqp_unmount();
        return EXIT_FAILURE;
    }

    // Allocate a buffer to hold directory entries.
    size_t count = MAX_ENTRIES;
    size_t buf_size = count * sizeof(nqp_dirent);
    void *buffer = malloc(buf_size);
    if (!buffer)
    {
        fprintf(stderr, "Memory allocation error\n");
        nqp_close(fd);
        nqp_unmount();
        return EXIT_FAILURE;
    }

    // Read directory entries.
    ssize_t bytes = nqp_getdents(fd, buffer, count);
    if (bytes < 0)
    {
        fprintf(stderr, "Failed to list directory entries for %s\n", argv[2]);
        free(buffer);
        nqp_close(fd);
        nqp_unmount();
        return EXIT_FAILURE;
    }

    printf("Read %zd bytes from directory fd=%d\n", bytes, fd);

    // Cast the buffer to an array of nqp_dirent entries.
    nqp_dirent *entries = (nqp_dirent *)buffer;
    size_t num_entries = bytes / sizeof(nqp_dirent);

    // Loop through and print each directory entry.
    for (size_t i = 0; i < num_entries; i++)
    {
        printf("Entry %zu:\n", i);
        printf("  Inode: %" PRIu64 "\n", entries[i].inode_number);
        printf("  Name: %s", entries[i].name);

        if (entries[i].type == DT_DIR)
        {
            printf(" (Directory)");
        }
        else
        {
            printf(" (Regular File)");
        }
        printf("\n  Name Length: %zu\n", entries[i].name_len);
    }

    // Clean up.
    free(buffer);
    nqp_close(fd);
    nqp_unmount();

    return EXIT_SUCCESS;
}
