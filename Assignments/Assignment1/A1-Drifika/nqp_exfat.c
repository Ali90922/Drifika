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

                        for (int slot = 0; slot < MAX_OPEN_FILES; slot++)
                        {
                            if (!open_files[slot].in_use)
                            {
                                open_files[slot].in_use = 1;
                                open_files[slot].start_cluster = file_cluster;
                                open_files[slot].file_size = file_size; // Consider using valid_data_length here if preferred
                                open_files[slot].offset = 0;            // Initial read offset is 0
                                break;
                            }
                        }
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

// Had to change Read to use the OFT for the Prof's cat to work

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
    // Check basic preconditions.
    if (!is_mounted || fd < 2 || !buffer || count == 0)
    {
        return -1; // Invalid parameters
    }

    // Look up the open file entry corresponding to the file descriptor.
    // (We assume that fd is the file’s starting cluster number.)
    open_file_entry *file = NULL;
    for (int i = 0; i < MAX_OPEN_FILES; i++)
    {
        if (open_files[i].in_use && open_files[i].start_cluster == (uint32_t)fd)
        {
            file = &open_files[i];
            break;
        }
    }
    if (file == NULL)
    {
        return -1; // The file is not open.
    }

    // If we have reached or passed the end of the file, return 0 (EOF).
    if (file->offset >= file->file_size)
    {
        return 0;
    }

    // Do not try to read beyond the file's end.
    if (count > file->file_size - file->offset)
    {
        count = file->file_size - file->offset;
    }

    // Calculate the size of a cluster.
    uint32_t cluster_size = (1 << mbr.bytes_per_sector_shift) *
                            (1 << mbr.sectors_per_cluster_shift);

    // Allocate a temporary buffer for one cluster.
    uint8_t *cluster_buffer = malloc(cluster_size);
    if (!cluster_buffer)
    {
        return -1; // Memory allocation failure.
    }

    size_t total_bytes_read = 0;  // Total bytes read so far.
    size_t bytes_to_read = count; // Bytes still needed to read.

    // Determine the starting point: which cluster and what offset in that cluster.
    size_t starting_offset = file->offset;
    uint32_t current_cluster = file->start_cluster;
    uint32_t clusters_to_skip = starting_offset / cluster_size;
    size_t offset_in_cluster = starting_offset % cluster_size;

    // Skip clusters that come entirely before the current offset.
    for (uint32_t i = 0; i < clusters_to_skip; i++)
    {
        uint64_t fat_entry_offset = (mbr.fat_offset * (1 << mbr.bytes_per_sector_shift)) +
                                    (current_cluster * sizeof(uint32_t));
        fseek(fs_image, fat_entry_offset, SEEK_SET);
        if (fread(&current_cluster, sizeof(uint32_t), 1, fs_image) != 1)
        {
            free(cluster_buffer);
            return -1;
        }
        if (current_cluster == 0xFFFFFFFF)
        { // Reached end-of-file chain prematurely.
            free(cluster_buffer);
            return total_bytes_read;
        }
    }

    // Now, read cluster-by-cluster.
    while (bytes_to_read > 0 && current_cluster != 0xFFFFFFFF)
    {
        // Calculate the absolute offset of the current cluster in the file system image.
        uint64_t cluster_addr = (mbr.cluster_heap_offset * (1 << mbr.bytes_per_sector_shift)) +
                                ((current_cluster - 2) * cluster_size);
        fseek(fs_image, cluster_addr, SEEK_SET);
        if (fread(cluster_buffer, cluster_size, 1, fs_image) != 1)
        {
            free(cluster_buffer);
            return -1;
        }

        // Determine how many bytes are available in the current cluster.
        size_t available_in_cluster = cluster_size - offset_in_cluster;
        size_t bytes_from_cluster = (bytes_to_read < available_in_cluster) ? bytes_to_read : available_in_cluster;

        // Copy the bytes from the cluster into the caller's buffer.
        memcpy((char *)buffer + total_bytes_read, cluster_buffer + offset_in_cluster, bytes_from_cluster);

        total_bytes_read += bytes_from_cluster;
        bytes_to_read -= bytes_from_cluster;
        // Update the file's offset.
        file->offset += bytes_from_cluster;

        // After the first cluster, subsequent clusters start reading at offset 0.
        offset_in_cluster = 0;

        if (bytes_to_read > 0)
        {
            // Follow the FAT chain to the next cluster.
            uint64_t fat_entry_offset = (mbr.fat_offset * (1 << mbr.bytes_per_sector_shift)) +
                                        (current_cluster * sizeof(uint32_t));
            fseek(fs_image, fat_entry_offset, SEEK_SET);
            if (fread(&current_cluster, sizeof(uint32_t), 1, fs_image) != 1)
            {
                free(cluster_buffer);
                return -1;
            }
        }
    }

    free(cluster_buffer);
    return total_bytes_read; // Return the number of bytes read.
}

ssize_t nqp_getdents(int fd, void *dirp, size_t count)
{
    // Here, 'count' is the number of nqp_dirent entries to read.
    if (!is_mounted || fd < 2 || !dirp || count < 1)
    {
        return -1;
    }

    // We'll only support count == 1 (i.e. one entry per call) for this simple ls.
    // Use static variables to keep track of our directory reading state.
    static uint32_t current_cluster = 0;
    static size_t current_index = 0;
    static int state_initialized = 0;

    // If state is not yet initialized (or if this is the first call for this directory),
    // initialize with the starting cluster (which is passed as fd) and reset index.
    if (!state_initialized)
    {
        current_cluster = fd;
        current_index = 0;
        state_initialized = 1;
    }

    // Allocate a temporary buffer to read one cluster.
    uint32_t cluster_size = (1 << mbr.bytes_per_sector_shift) * (1 << mbr.sectors_per_cluster_shift);
    uint8_t *cluster_buffer = malloc(cluster_size);
    if (!cluster_buffer)
    {
        return -1;
    }

    nqp_dirent *result_entry = (nqp_dirent *)dirp; // This is where we'll store the one entry to return.
    int found = 0;

    // Loop until we find one valid directory entry or we reach the end.
    while (!found && current_cluster != 0xFFFFFFFF)
    {
        // Compute the byte offset for the current cluster.
        uint64_t cluster_offset = (mbr.cluster_heap_offset * (1 << mbr.bytes_per_sector_shift)) +
                                  (current_cluster - 2) * cluster_size;
        fseek(fs_image, cluster_offset, SEEK_SET);
        if (fread(cluster_buffer, cluster_size, 1, fs_image) != 1)
        {
            free(cluster_buffer);
            return -1; // Error reading from the file system.
        }

        size_t num_entries = cluster_size / sizeof(directory_entry);
        directory_entry *entries = (directory_entry *)cluster_buffer;

        // Iterate over the entries in this cluster, starting at current_index.
        for (size_t i = current_index; i < num_entries; i++)
        {
            // If we encounter an end-of-directory marker, we're done.
            if (entries[i].entry_type == DENTRY_TYPE_END)
            {
                // Reset state for next directory read.
                state_initialized = 0;
                free(cluster_buffer);
                return 0;
            }

            // Look for a valid entry set starting with a FILE entry.
            if (entries[i].entry_type == DENTRY_TYPE_FILE)
            {
                // Check that the next two entries exist.
                if (i + 2 >= num_entries)
                    break; // Not enough entries in this cluster; we'll fetch the next cluster.

                // Verify that we have a STREAM_EXTENSION and a FILE_NAME following.
                if (entries[i + 1].entry_type != DENTRY_TYPE_STREAM_EXTENSION ||
                    entries[i + 2].entry_type != DENTRY_TYPE_FILE_NAME)
                {
                    continue; // Not a valid entry set; skip.
                }

                // Convert the Unicode file name (15 uint16_t characters) to ASCII.
                char *ascii_name = unicode2ascii(entries[i + 2].file_name.file_name, 15);
                if (!ascii_name)
                    continue;

                // Populate the nqp_dirent structure.
                result_entry->inode_number = entries[i + 1].stream_extension.first_cluster;
                result_entry->name = ascii_name;
                result_entry->name_len = strlen(ascii_name);
                // Use a flag (here 0x10) to indicate a directory—adjust as needed.
                if (entries[i].file.file_attributes & 0x10)
                {
                    result_entry->type = DT_DIR;
                }
                else
                {
                    result_entry->type = DT_REG;
                }

                // Update our state: advance the index beyond the processed entry set.
                current_index = i + 3;
                found = 1;
                break;
            }
        }

        if (!found)
        {
            // We did not find a valid entry in this cluster.
            // Reset current_index for the next cluster.
            current_index = 0;
            // Read the FAT entry to get the next cluster in the directory chain.
            uint64_t fat_entry_offset = (mbr.fat_offset * (1 << mbr.bytes_per_sector_shift)) +
                                        (current_cluster * sizeof(uint32_t));
            fseek(fs_image, fat_entry_offset, SEEK_SET);
            if (fread(&current_cluster, sizeof(uint32_t), 1, fs_image) != 1)
            {
                free(cluster_buffer);
                return -1;
            }
        }
    }

    free(cluster_buffer);
    if (found)
        return sizeof(nqp_dirent); // Return the number of bytes (one entry).
    else
        return 0;
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

// Function that Prints the Open File Table :
void print_open_file_table(void)
{
    printf("Open File Table:\n");
    for (int i = 0; i < MAX_OPEN_FILES; i++)
    {
        if (open_files[i].in_use)
        {
            printf("Slot %d: IN USE\n", i);
            printf("   Start Cluster: %u\n", open_files[i].start_cluster);
            printf("   File Size: %llu bytes\n", (unsigned long long)open_files[i].file_size);
            printf("   Current Offset: %llu bytes\n", (unsigned long long)open_files[i].offset);
        }
        else
        {
            printf("Slot %d: FREE\n", i);
        }
    }
}

// Function that returns File using using FD
int FileSize(int FD)
{
    for (int i = 0; i < MAX_OPEN_FILES; i++)
    {
        if (open_files[i].start_cluster == (uint32_t)FD)
        {
            return open_files[i].file_size;
            break;
        }
    }
    return -1;
}

// Birds of the Feather
