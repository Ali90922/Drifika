#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "nqp_io.h"
#include "nqp_exfat_types.h"

// Global state for the mounted file system
static FILE *fs_image = NULL; // File pointer for the file system image
static int is_mounted = 0;    // Flag to indicate if the FS is mounted
static main_boot_record mbr;  // Stores the Main Boot Record data

typedef struct
{
    int in_use;
    uint32_t start_cluster;
    uint64_t file_size; // valid_data_length
    uint64_t offset;    // current offset in the file
} open_file_entry;

#define MAX_OPEN_FILES 8
open_file_entry open_files[MAX_OPEN_FILES];

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

    // Post the open File to the OFT
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
 *
 * Parameters:
 *  * fd: The file descriptor to close. Must be a nonnegative integer.
 * Return: -1 on error or 0 on success.
 */
int nqp_close(int fd)
{
    if (!is_mounted || fd < 0)
    {
        return -1; // Invalid file descriptor
    }

    // If using an Open File Table (OFT), mark the file as closed
    // For now, just return success since we are not tracking open files
    return 0;
}

/**
 * Read data from a file.
 */
/**
 * Read from a file desriptor.
 *
 * Parameters:
 *  * fd: The file descriptor to read from. Must be a nonnegative integer. The
 *        file descriptor should refer to a file, not a directory.
 *  * buffer: The buffer to read data into. Must not be NULL.
 *  * count: The number of bytes to read into the buffer.
 * Return: The number of bytes read, 0 at the end of the file, or -1 on error.
 */
ssize_t nqp_read(int fd, void *buffer, size_t count)
{
    if (!is_mounted || fd < 2 || !buffer || count == 0)
    {
        return -1; // Invalid parameters
    }

    uint32_t current_cluster = fd; // fd is the first cluster number

    // The left shift in the single bit is to raise the value to the power of 2 :
    uint32_t cluster_size = (1 << mbr.bytes_per_sector_shift) * (1 << mbr.sectors_per_cluster_shift); // Total bytes per cluster

    size_t bytes_read = 0;

    size_t bytes_to_read = count;
    uint8_t *cluster_buffer = malloc(cluster_size);

    if (!cluster_buffer)
    {
        return -1; // Memory allocation failure
    }

    while (bytes_to_read > 0 && current_cluster != 0xFFFFFFFF) // 0xFFFFFFFF = End of file
    {
        // Compute the offset in the exFAT image
        uint64_t cluster_offset = (mbr.cluster_heap_offset * (1 << mbr.bytes_per_sector_shift)) + (current_cluster - 2) * cluster_size;

        // cluster_heap_offset is the start location of the file data in the exFAT
        // current_cluster - 2 is there to convert the cluster number to index - since cluster numbers start from 2.
        // So the final offset being calculated is : (Start of Cluster Heap) + (Offset of current cluster)

        fseek(fs_image, cluster_offset, SEEK_SET);
        if (fread(cluster_buffer, cluster_size, 1, fs_image) != 1)
        {
            free(cluster_buffer);
            return -1; // Error reading from file system
        }

        // Read the requested bytes
        size_t bytes_from_cluster = (bytes_to_read < cluster_size) ? bytes_to_read : cluster_size;
        memcpy((char *)buffer + bytes_read, cluster_buffer, bytes_from_cluster);

        bytes_read += bytes_from_cluster;
        bytes_to_read -= bytes_from_cluster;

        // Move to the next cluster in the file
        fseek(fs_image, (mbr.fat_offset * (1 << mbr.bytes_per_sector_shift)) + (current_cluster * sizeof(uint32_t)), SEEK_SET);
        fread(&current_cluster, sizeof(uint32_t), 1, fs_image);
    }

    free(cluster_buffer);
    return bytes_read; // Return number of bytes read
}

/**
 * Get directory entries for a directory.
 */

/**
 * Get the directory entries for a directory. Similar to read()ing a file, you
 * may need to call this function repeatedly to get all directory entries.
 *
 * Parameters:
 *  * fd: The file descriptor to read from. Must be a nonnegative integer. The
 *        file descriptor should refer to a directory, not a file.
 *  * dirp: the buffer into which the directory entries will be written. The
 *          buffer must not be NULL.
 *  * count: the size of the buffer. Must be at least sizeof(nqp_dirent).
 * Return: The total number of bytes read into the buffer, 0 at the end of the
 * directory, or -1 on error.
 */
ssize_t nqp_getdents(int fd, void *dirp, size_t count)
{
    // Validate input parameters
    if (!is_mounted || fd < 2 || !dirp || count < sizeof(nqp_dirent))
    {
        return -69;
    }

    // Calculate cluster size in bytes
    uint32_t cluster_size = (1 << mbr.bytes_per_sector_shift) * (1 << mbr.sectors_per_cluster_shift);
    uint8_t *cluster_buffer = malloc(cluster_size);
    if (!cluster_buffer)
    {
        return -1; // Memory allocation failure
    }

    // 'dentries' will point to successive nqp_dirent structures in the caller's buffer.
    nqp_dirent *dentries = (nqp_dirent *)dirp;
    size_t bytes_written = 0;

    // Start reading from the directory cluster (fd is the starting cluster number)
    uint32_t current_cluster = fd;
    int done = 0;

    while (!done && current_cluster != 0xFFFFFFFF)
    {
        // Compute the absolute byte offset of the current cluster in the image
        uint64_t cluster_offset = (mbr.cluster_heap_offset * (1 << mbr.bytes_per_sector_shift)) +
                                  (current_cluster - 2) * cluster_size;
        fseek(fs_image, cluster_offset, SEEK_SET);
        if (fread(cluster_buffer, cluster_size, 1, fs_image) != 1)
        {
            free(cluster_buffer);
            return -1; // Error reading from the file system
        }

        // Interpret the cluster data as an array of directory_entry structures.
        size_t num_entries = cluster_size / sizeof(directory_entry);
        directory_entry *entry = (directory_entry *)cluster_buffer;

        // Iterate over each directory entry in the cluster
        for (size_t i = 0; i < num_entries; i++)
        {
            // If we encounter an end-of-directory marker, stop processing.
            if (entry[i].entry_type == DENTRY_TYPE_END)
            {
                done = 1;
                break;
            }

            // We assume that a valid file (or directory) entry set starts with a FILE entry,
            // followed by a STREAM_EXTENSION entry and then one or more FILE_NAME entries.
            if (entry[i].entry_type == DENTRY_TYPE_FILE)
            {
                // Basic sanity check: make sure the following two entries exist.
                if (i + 2 >= num_entries)
                    continue; // Not enough entries left in this cluster; ideally, you would handle spanning clusters.

                // Check that the next entry is a STREAM_EXTENSION and the one after is a FILE_NAME.
                if (entry[i + 1].entry_type != DENTRY_TYPE_STREAM_EXTENSION ||
                    entry[i + 2].entry_type != DENTRY_TYPE_FILE_NAME)
                {
                    continue; // Skip invalid or incomplete entry set
                }

                // Convert the Unicode file name from the FILE_NAME entry to an ASCII string.
                // Note: The file_name entry has a field 'file_name' that is an array of 15 uint16_t values.
                char *ascii_name = unicode2ascii(entry[i + 2].file_name.file_name, 15);
                if (!ascii_name)
                    continue;

                // Populate the nqp_dirent structure.
                // For inode_number, we use the first_cluster from the STREAM_EXTENSION.
                dentries->inode_number = entry[i + 1].stream_extension.first_cluster;
                // The name pointer is set to the allocated ASCII string.
                dentries->name = ascii_name;
                // Record the length of the name.
                dentries->name_len = strlen(ascii_name);
                // Determine the file type.
                // For example, if a certain bit is set in file_attributes indicating a directory, mark it as DT_DIR.
                // (Here, we simply use DT_REG for file entries and DT_DIR for directories, based on a hypothetical flag.)
                if (entry[i].file.file_attributes & 0x10) // Replace 0x10 with the proper directory flag if needed.
                {
                    dentries->type = DT_DIR;
                }
                else
                {
                    dentries->type = DT_REG;
                }

                // Advance the output pointer and update the number of bytes written.
                bytes_written += sizeof(nqp_dirent);
                dentries++; // Move to the next slot in the caller's buffer.

                // Skip the next two entries that we already processed.
                i += 2;

                // Check if the buffer is full.
                if (bytes_written + sizeof(nqp_dirent) > count)
                {
                    free(cluster_buffer);
                    return bytes_written;
                }
            }
        }

        // Move to the next cluster in the directory's FAT chain.
        uint64_t fat_entry_offset = (mbr.fat_offset * (1 << mbr.bytes_per_sector_shift)) +
                                    (current_cluster * sizeof(uint32_t));
        fseek(fs_image, fat_entry_offset, SEEK_SET);
        if (fread(&current_cluster, sizeof(uint32_t), 1, fs_image) != 1)
        {
            free(cluster_buffer);
            return -1;
        }
    }

    free(cluster_buffer);

    return bytes_written; // Return the total number of bytes written to the buffer.
}

// Problems :

// 1. Problems while accessing/ opening/ reading Nested Files -- Fix it -- Problem Fixed
// Above Problem is with not having the appropriate file extensions -- properly name ur files like .md or .txt extensions

// Problems wiht this function -- Better idea to implement an OFT
// This function is trash -- come back to this
int nqp_size(int fd)
{
    if (fd < 2) // Invalid file descriptor (should be a valid cluster number)
    {
        return -1;
    }

    // Compute cluster size in bytes
    uint32_t cluster_size = (1 << mbr.bytes_per_sector_shift) * (1 << mbr.sectors_per_cluster_shift);

    // Allocate buffer for directory reading
    uint8_t *cluster_buffer = malloc(cluster_size);
    if (!cluster_buffer)
    {
        return -1;
    }

    // Locate Root Directory Cluster
    uint32_t root_cluster = mbr.first_cluster_of_root_directory;
    uint32_t cluster_offset = mbr.cluster_heap_offset * (1 << mbr.bytes_per_sector_shift);
    uint64_t cluster_address = cluster_offset + (root_cluster - 2) * cluster_size;

    // Seek to the Root Directory
    if (fseek(fs_image, cluster_address, SEEK_SET) != 0)
    {
        perror("Error seeking to root directory cluster");
        free(cluster_buffer);
        return -1;
    }

    // Read the Directory Cluster
    size_t bytes_read = fread(cluster_buffer, 1, cluster_size, fs_image);
    if (bytes_read != cluster_size)
    {
        perror("Error reading root directory cluster");
        free(cluster_buffer);
        return -1;
    }

    // Scan Directory Entries for the File
    directory_entry *entry = (directory_entry *)cluster_buffer;
    stream_extension *stream_ext = NULL;

    for (size_t i = 0; i < cluster_size / sizeof(directory_entry); i++)
    {
        if (entry[i].entry_type == DENTRY_TYPE_FILE) // Found a file entry
        {
            // The Stream Extension entry follows immediately
            stream_ext = (stream_extension *)&entry[i + 1];

            // Check if this file's first cluster matches `fd`
            if (stream_ext->first_cluster == (uint32_t)fd)
            {
                // Found the correct file, extract file size
                uint64_t file_size = stream_ext->data_length;

                // Check for integer overflow
                if (file_size > INT_MAX)
                {
                    fprintf(stderr, "Error: File size exceeds integer limit\n");
                    free(cluster_buffer);
                    return -1;
                }

                free(cluster_buffer);
                return (int)file_size; // Return file size as an int
            }
        }
    }

    // If no matching file was found
    fprintf(stderr, "Error: File with cluster %d not found\n", fd);
    free(cluster_buffer);
    return -1;
}
