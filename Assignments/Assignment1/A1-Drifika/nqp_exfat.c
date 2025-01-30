#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "nqp_io.h"
#include "nqp_exfat_types.h"

// Global state for the mounted file system
static FILE *fs_image = NULL; // File pointer for the file system image
static int is_mounted = 0;    // Flag to indicate if the FS is mounted
static main_boot_record mbr;  // Stores the Main Boot Record data

/**
 * Convert a Unicode-formatted string containing only ASCII characters
 * into a regular ASCII-formatted string (16-bit chars to 8-bit chars).
 */
char *unicode2ascii(uint16_t *unicode_string, uint8_t length)
{
    assert(unicode_string != NULL);
    assert(length > 0);

    char *ascii_string = calloc(length + 1, sizeof(char)); // Allocate memory for ASCII string
    if (ascii_string)
    {
        for (uint8_t i = 0; i < length; i++)
        {
            ascii_string[i] = (char)(unicode_string[i] & 0xFF); // Strip the top 8 bits
        }
        ascii_string[length] = '\0'; // Null-terminate the string
    }
    return ascii_string;
}

/**
 * Mount the file system.
 *  What Does nqp_mount Need to Do?
 * 1. Open the file system image (a binary file representing an exFAT-formatted disk).
 * 2. Read the Main Boot Record (MBR) from the first sector.
 * 3. Validate the following fields:
 *       File System Name (fs_name): Must be "EXFAT".
         MustBeZero (must_be_zero): Must be all zero bytes.
         First Cluster of Root Directory (first_cluster_of_root_directory): Must be ≥ 2 (since cluster numbers start at 2).

   4. Perform additional sanity checks :
       Ensure the Boot Signature (boot_signature) is 0xAA55.
       Ensure the volume size is reasonable.
       Ensure the FAT Table is properly located.


       // Just do these things for full marks :
       Specifically checks the following fields for validity

        FileSystemName
        MustBeZero
F       FirstClusterOfRootDirectory

 */
nqp_error nqp_mount(const char *source, nqp_fs_type fs_type)
{

    // Validate input parameters
    if (!source)
    {
        return NQP_INVAL;
    }
    if (fs_type != NQP_FS_EXFAT)
    {
        return NQP_UNSUPPORTED_FS;
    }

    // Open the file system image
    fs_image = fopen(source, "rb");
    if (!fs_image)
    {
        return NQP_INVAL;
    }

    // Read the Main Boot Record
    if (fread(&mbr, sizeof(main_boot_record), 1, fs_image) != 1)
    {
        fclose(fs_image);
        fs_image = NULL;
        return NQP_FSCK_FAIL;
    }

    // Validate the file system name and boot signature
    if (strncmp(mbr.fs_name, "EXFAT   ", 8) != 0 || mbr.boot_signature != 0xAA55)
    {
        fclose(fs_image);
        fs_image = NULL;
        return NQP_FSCK_FAIL;
    }

    // Check the Must be zero thing
    // The must_be_zero field in the exFAT Main Boot Record is a section of 53 bytes that must be entirely zero.
    // If any byte in this field is non-zero, the file system is considered corrupt.
    // Validate must_be_zero field (Ensure all 53 bytes are zero)
    for (size_t i = 0; i < sizeof(mbr.must_be_zero); i++)
    {
        if (mbr.must_be_zero[i] != 0)
        { // If any byte is non-zero, the file system is corrupt
            printf("ERROR: must_be_zero field is not all zero!\n");
            fclose(fs_image);
            fs_image = NULL;
            return NQP_FSCK_FAIL;
        }
    }

    // Validate FirstClusterOfRootDirectory

    // Validate `first_cluster_of_root_directory`
    if (mbr.first_cluster_of_root_directory < 2)
    {
        printf("ERROR: Invalid FirstClusterOfRootDirectory: %u\n", mbr.first_cluster_of_root_directory);
        fclose(fs_image);
        fs_image = NULL;
        return NQP_FSCK_FAIL;
    }

    // Set the mounted state
    is_mounted = 1;
    return NQP_OK;
}

/**
 * Unmount the file system.
 */

/**
 * "Unmount" the mounted file system.
 *
 * This function should be called to flush any changes to the file system's
 * volume (there shouldn't be! All operations are read only.)
 *
 * Return: NQP_INVAL on error (e.g., there is no fs currently mounted) or
 *         NQP_OK on success.
 */
nqp_error nqp_unmount(void)
{
    if (!is_mounted || !fs_image)
    {
        return NQP_INVAL;
    }
    fclose(fs_image);
    fs_image = NULL;
    is_mounted = 0;
    return NQP_OK;
}

/**
 * Open a file in the mounted file system.
 */

/**
 * Open the file at pathname in the "mounted" file system.
 *
 * Parameters:
 *  * pathname: The path of the file or directory in the file system that
 *              should be opened.  Must not be NULL.
 * Return: -1 on error, or a nonnegative integer on success. The nonnegative
 *         integer is a file descriptor.
 */
int nqp_open(const char *pathname)
{
    if (!is_mounted || !pathname)
        return -1;

    uint32_t current_cluster = mbr.first_cluster_of_root_directory;
    uint32_t file_cluster = 0;
    uint64_t file_size = 0;
    uint32_t create_time, modify_time, access_time;
    uint16_t file_attributes;

    size_t cluster_size = (1 << mbr.bytes_per_sector_shift) * (1 << mbr.sectors_per_cluster_shift);
    uint8_t *cluster_buffer = malloc(cluster_size);
    if (!cluster_buffer)
        return -1;

    char path_copy[256];
    strncpy(path_copy, pathname, sizeof(path_copy));
    char *token = strtok(path_copy, "/");

    while (token != NULL)
    {
        int found = 0;

        while (current_cluster != 0xFFFFFFFF)
        { // Traverse FAT chain
            uint32_t cluster_offset = mbr.cluster_heap_offset * (1 << mbr.bytes_per_sector_shift);
            uint64_t cluster_address = cluster_offset + (current_cluster - 2) * cluster_size;

            fseek(fs_image, cluster_address, SEEK_SET);
            if (fread(cluster_buffer, cluster_size, 1, fs_image) != 1)
            {
                free(cluster_buffer);
                return -1;
            }

            directory_entry *entry = (directory_entry *)cluster_buffer;
            for (size_t i = 0; i < cluster_size / sizeof(directory_entry); i++)
            {
                if (entry[i].entry_type == DENTRY_TYPE_FILE)
                {
                    char *ascii_filename = unicode2ascii(entry[i + 2].file_name.file_name, 15);
                    if (ascii_filename && strcmp(ascii_filename, token) == 0)
                    {
                        file_cluster = entry[i + 1].stream_extension.first_cluster;
                        file_size = entry[i + 1].stream_extension.data_length;
                        file_attributes = entry[i].file.file_attributes;
                        create_time = entry[i].file.create_timestamp;
                        modify_time = entry[i].file.last_modified_timestamp;
                        access_time = entry[i].file.last_accessed_timestamp;

                        // ✅ Print file metadata
                        printf("\nOpened file: %s\n", pathname);
                        printf("File Descriptor (First Cluster): %u\n", file_cluster);
                        printf("File Size: %llu bytes\n", (unsigned long long)file_size);
                        printf("File Attributes: 0x%X\n", file_attributes);
                        printf("Created: %u, Modified: %u, Accessed: %u\n", create_time, modify_time, access_time);

                        free(ascii_filename);
                        found = 1;
                        break;
                    }
                    free(ascii_filename);
                }
            }

            if (found)
                break;

            // Move to next cluster in the directory using the FAT table
            fseek(fs_image, mbr.fat_offset * (1 << mbr.bytes_per_sector_shift) + current_cluster * sizeof(uint32_t), SEEK_SET);
            fread(&current_cluster, sizeof(uint32_t), 1, fs_image);
        }

        if (!found)
        {
            free(cluster_buffer);
            return -1; // File not found
        }

        token = strtok(NULL, "/");
        if (token)
            current_cluster = file_cluster;
    }

    free(cluster_buffer);
    return file_cluster; // Return first cluster as file descriptor
}

/**
 * Close the file referred to by the descriptor.
 */
int nqp_close(int fd)
{
    if (!is_mounted || fd < 0)
    {
        return -1;
    }

    // Placeholder: Close the file (not fully implemented)
    return 0;
}

/**
 * Read data from a file.
 */
ssize_t nqp_read(int fd, void *buffer, size_t count)
{
    if (!is_mounted || fd < 0 || !buffer || count == 0)
    {
        return -1;
    }

    // Placeholder: Read data from the file (not fully implemented)
    size_t bytes_read = 0; // Replace with actual read logic
    return bytes_read;
}

/**
 * Get directory entries for a directory.
 */
ssize_t nqp_getdents(int fd, void *dirp, size_t count)
{
    if (!is_mounted || fd < 0 || !dirp || count < sizeof(nqp_dirent))
    {
        return -1;
    }

    // Placeholder: Retrieve directory entries (not fully implemented)
    return 0; // Return total bytes written to the buffer
}
