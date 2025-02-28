#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <errno.h>

/* Define error codes for the NQP operations */
typedef enum {
    NQP_SUCCESS = 0,
    NQP_INVALID_PARAM = -1,
    NQP_UNSUPPORTED   = -2,
    NQP_ERROR         = -3
} nqp_error;

/* Define filesystem type for NQP */
typedef enum {
    NQP_FS_EXFAT = 1
} nqp_fs_type;

/* Maximum number of open files */
#define MAX_OPEN_FILES 256

/* Structure representing an open file */
typedef struct {
    int is_open;
    int is_dir;
    size_t current_offset;
} open_file;

/* Global open file table */
open_file open_file_table[MAX_OPEN_FILES];

/* Structure representing the master boot record or filesystem metadata */
typedef struct {
    uint32_t cluster_count;
    uint32_t first_cluster_of_root_directory;
} mbr_t;

/* Global mbr variable */
mbr_t mbr = { .cluster_count = 10000, .first_cluster_of_root_directory = 2 };

/* Structure representing a stream extension */
typedef struct {
    uint32_t first_cluster;
    uint32_t name_length; /* typically used to calculate number of filename parts */
} stream_extension;

/* Dummy structure representing a file */
typedef struct {
    /* File-specific data can be added here */
    int dummy;
} file_t;

/* Structure representing an entry set */
typedef struct {
    stream_extension *stream_extension;
    file_t *file;
    char **filenames;
    uint32_t file_name_count;
} entry_set;

/* 
 * Function: nqp_mount
 * -------------------
 * Mounts the exFAT filesystem.
 *
 * Preconditions:
 *   - pathname != NULL
 *   - fs_type == NQP_FS_EXFAT
 *
 * Returns:
 *   - NQP_SUCCESS on success, error code otherwise.
 */
nqp_error nqp_mount(const char *pathname, nqp_fs_type fs_type)
{
    /* Check that pathname is not NULL */
    if (pathname == NULL) {
        fprintf(stderr, "Error: pathname is NULL in %s\n", __func__);
        return NQP_INVALID_PARAM;
    }
    /* Check that the filesystem type is supported */
    if (fs_type != NQP_FS_EXFAT) {
        fprintf(stderr, "Error: Unsupported FS type in %s\n", __func__);
        return NQP_UNSUPPORTED;
    }
    /* 
       Here the actual mounting logic would be implemented.
       For this translation, we simply return a success code.
    */
    return NQP_SUCCESS;
}

/* 
 * Function: nqp_open
 * ------------------
 * Opens a file given by the source path.
 *
 * Preconditions:
 *   - source != NULL
 *   - Upon success fd >= 0.
 *
 * Returns:
 *   - A non-negative file descriptor on success or -1 on error.
 */
int nqp_open(const char *source)
{
    if (source == NULL) {
        fprintf(stderr, "Error: source is NULL in %s\n", __func__);
        return -1;
    }
    /* Dummy implementation: always use file descriptor 0 */
    int fd = 0;
    if (fd < MAX_OPEN_FILES) {
        open_file_table[fd].is_open = 1;
        open_file_table[fd].is_dir = 0;
        open_file_table[fd].current_offset = 0;
    } else {
        fprintf(stderr, "Error: Exceeded maximum open files in %s\n", __func__);
        return -1;
    }
    return fd;
}

/* 
 * Function: nqp_read
 * ------------------
 * Reads data from an open file.
 *
 * Preconditions:
 *   - fd < MAX_OPEN_FILES
 *   - buffer != NULL
 *   - open_file_table[fd].is_open
 *   - !open_file_table[fd].is_dir
 *
 * The resulting current_offset is verified to be:
 *   original_offset + total_read.
 *
 * Returns:
 *   - The number of bytes read (as ssize_t) or -1 on error.
 */
ssize_t nqp_read(int fd, void *buffer, size_t count)
{
    if (fd < 0 || fd >= MAX_OPEN_FILES) {
        fprintf(stderr, "Error: file descriptor out of range in %s\n", __func__);
        return -1;
    }
    if (buffer == NULL) {
        fprintf(stderr, "Error: buffer is NULL in %s\n", __func__);
        return -1;
    }
    if (!open_file_table[fd].is_open) {
        fprintf(stderr, "Error: file not open in %s\n", __func__);
        return -1;
    }
    if (open_file_table[fd].is_dir) {
        fprintf(stderr, "Error: attempted read on a directory in %s\n", __func__);
        return -1;
    }
    /* Dummy implementation: pretend to read 'count' bytes */
    ssize_t total_read = count;
    size_t original_offset = open_file_table[fd].current_offset;
    open_file_table[fd].current_offset = original_offset + total_read;
    return total_read;
}

/* 
 * Function: nqp_getdents
 * ----------------------
 * Retrieves directory entries.
 *
 * Preconditions:
 *   - dirp (buffer) != NULL
 *   - count > 0
 *
 * Additionally, if an entry is a volume label then:
 *   entry.entry_type == DENTRY_TYPE_VOLUME_LABEL.
 *
 * Returns:
 *   - The number of bytes read or -1 on error.
 */
ssize_t nqp_getdents(int fd, void *buffer, size_t count)
{
    if (buffer == NULL) {
        fprintf(stderr, "Error: buffer is NULL (dirp) in %s\n", __func__);
        return -1;
    }
    if (count == 0) {
        fprintf(stderr, "Error: count must be greater than 0 in %s\n", __func__);
        return -1;
    }
    /* Dummy implementation: return 0 indicating no directory entries */
    return 0;
}

/* 
 * Function: nqp_vol_label
 * -----------------------
 * Returns the volume label.
 *
 * Preconditions:
 *   - first_cluster < mbr.cluster_count
 *
 * Returns:
 *   - A dynamically allocated string containing the volume label.
 */
char *nqp_vol_label(void)
{
    char *label = strdup("NQP_VOL");
    return label;
}

/* 
 * Function: entrysets
 * -------------------
 * Generates an array (entry_set pointer) from stream_extension data and a parameter.
 *
 * Preconditions:
 *   - file_name_count == 0 initially.
 *   - sets[set_count - 1].file != NULL.
 *   - For non-zero file_name_count, sets[set_count - 1].stream_extension != NULL.
 *
 * Returns:
 *   - A pointer to an allocated entry_set.
 */
entry_set *entrysets(stream_extension *se, uint32_t param)
{
    (void)param; /* Unused parameter in dummy implementation */
    entry_set *sets = (entry_set *)calloc(1, sizeof(entry_set));
    if (sets == NULL) {
        fprintf(stderr, "Error: Memory allocation failure in %s\n", __func__);
        exit(EXIT_FAILURE);
    }
    sets->stream_extension = se;
    sets->file = (file_t *)malloc(sizeof(file_t));
    if (sets->file == NULL) {
        fprintf(stderr, "Error: Memory allocation failure for file in %s\n", __func__);
        free(sets);
        exit(EXIT_FAILURE);
    }
    sets->filenames = (char **)calloc(1, sizeof(char *));
    sets->file_name_count = (se && se->name_length > 0) ? ((se->name_length / 15) + 1) : 0;
    return sets;
}

/* 
 * Function: unicode2ascii
 * -----------------------
 * Converts a Unicode string (array of uint16_t) to an ASCII string.
 *
 * Preconditions:
 *   - length > 0
 *   - unicode_string != NULL
 *
 * Returns:
 *   - A dynamically allocated ASCII string.
 */
char *unicode2ascii(uint16_t *unicode_string, uint8_t length)
{
    if (length == 0) {
        fprintf(stderr, "Error: length is 0 in %s\n", __func__);
        return NULL;
    }
    if (unicode_string == NULL) {
        fprintf(stderr, "Error: unicode_string is NULL in %s\n", __func__);
        return NULL;
    }
    char *ascii = (char *)malloc(length + 1);
    if (ascii == NULL) {
        fprintf(stderr, "Error: Memory allocation failure in %s\n", __func__);
        exit(EXIT_FAILURE);
    }
    for (uint8_t i = 0; i < length; i++) {
        ascii[i] = (char)(unicode_string[i] & 0xFF);
    }
    ascii[length] = '\0';
    return ascii;
}

/* 
 * Function: validate_entry_set
 * ----------------------------
 * Validates an entry_set.
 *
 * Preconditions for each element in the set:
 *   - set[i].stream_extension != NULL
 *   - set[i].stream_extension->first_cluster >= 2
 *   - set[i].stream_extension->first_cluster <= (mbr.cluster_count + 1)
 *   - set[i].stream_extension->first_cluster == mbr.first_cluster_of_root_directory
 *   - set[i].file != NULL
 *   - set[i].filenames != NULL
 *   - set[i].file_name_count == ( (set[i].stream_extension->name_length / 15) + 1)
 */
void validate_entry_set(const entry_set *set)
{
    if (set == NULL) {
        fprintf(stderr, "Error: entry_set pointer is NULL in %s\n", __func__);
        return;
    }
    if (set->stream_extension == NULL) {
        fprintf(stderr, "Error: stream_extension is NULL in %s\n", __func__);
        return;
    }
    if (set->stream_extension->first_cluster < 2 ||
        set->stream_extension->first_cluster > (mbr.cluster_count + 1)) {
        fprintf(stderr, "Error: first_cluster out of range in %s\n", __func__);
        return;
    }
    if (set->stream_extension->first_cluster != mbr.first_cluster_of_root_directory) {
        fprintf(stderr, "Error: first_cluster does not match root directory first_cluster in %s\n", __func__);
        return;
    }
    if (set->file == NULL) {
        fprintf(stderr, "Error: file pointer is NULL in %s\n", __func__);
        return;
    }
    if (set->filenames == NULL) {
        fprintf(stderr, "Error: filenames pointer is NULL in %s\n", __func__);
        return;
    }
    uint32_t expected = (set->stream_extension->name_length / 15) + 1;
    if (set->file_name_count != expected) {
        fprintf(stderr, "Error: file_name_count (%u) does not equal expected (%u) in %s\n",
                set->file_name_count, expected, __func__);
        return;
    }
}

/* 
 * Function: next_cluster_in_chain
 * -------------------------------
 * Returns the next cluster in the chain given a stream_extension and a current cluster.
 *
 * Preconditions:
 *   - cluster >= 2
 *   - cluster <= mbr.cluster_count + 1
 *
 * Additionally, the next_cluster_number must satisfy:
 *   - next_cluster_number >= 2
 *   - next_cluster_number <= mbr.cluster_count + 1 || next_cluster_number == 0xffffffff
 *
 * Returns:
 *   - The next cluster number or 0 if there is an error.
 */
uint32_t next_cluster_in_chain(stream_extension *se, uint32_t cluster)
{
    (void)se; /* Unused in dummy implementation */
    if (cluster < 2 || cluster > mbr.cluster_count + 1) {
        fprintf(stderr, "Error: cluster out of range in %s\n", __func__);
        return 0;
    }
    /* Dummy implementation: simply return cluster+1, or 0xffffffff if end reached */
    uint32_t next_cluster_number = cluster + 1;
    if (next_cluster_number > mbr.cluster_count + 1) {
        next_cluster_number = 0xffffffff;
    }
    if (next_cluster_number != 0xffffffff &&
       (next_cluster_number < 2 || next_cluster_number > mbr.cluster_count + 1)) {
        fprintf(stderr, "Error: next_cluster_number out of range in %s\n", __func__);
        return 0;
    }
    return next_cluster_number;
}

/* 
 * Function: nqp_unmount
 * ---------------------
 * Unmounts the exFAT filesystem.
 *
 * Returns:
 *   - NQP_SUCCESS on success.
 */
nqp_error nqp_unmount(void)
{
    /* Dummy implementation */
    return NQP_SUCCESS;
}

/* 
 * Function: nqp_close
 * -------------------
 * Closes an open file descriptor.
 *
 * Preconditions:
 *   - fd < MAX_OPEN_FILES
 *
 * Returns:
 *   - 0 on success, or -1 on error.
 */
int nqp_close(int fd)
{
    if (fd < 0 || fd >= MAX_OPEN_FILES) {
        fprintf(stderr, "Error: file descriptor out of range in %s\n", __func__);
        return -1;
    }
    open_file_table[fd].is_open = 0;
    return 0;
}

/* A main function to demonstrate dummy calls of the above APIs */
int main(void)
{
    nqp_error err;
    int fd;
    ssize_t bytes;
    char buffer[128];
    char *vol_label;
    entry_set *es;
    uint16_t sample_unicode[] = { 'H', 'e', 'l', 'l', 'o' };
    char *ascii_str;
    stream_extension se = { .first_cluster = mbr.first_cluster_of_root_directory, .name_length = 15 };

    /* Mount the file system */
    err = nqp_mount("/mnt/nqp", NQP_FS_EXFAT);
    if (err != NQP_SUCCESS) {
        fprintf(stderr, "nqp_mount failed with error code %d\n", err);
        return EXIT_FAILURE;
    }

    /* Open a file */
    fd = nqp_open("file.txt");
    if (fd < 0) {
        fprintf(stderr, "nqp_open failed\n");
        return EXIT_FAILURE;
    }

    /* Read from the file */
    bytes = nqp_read(fd, buffer, sizeof(buffer));
    if (bytes < 0) {
        fprintf(stderr, "nqp_read failed\n");
        return EXIT_FAILURE;
    }
    printf("Read %zd bytes from file descriptor %d\n", bytes, fd);

    /* Get volume label */
    vol_label = nqp_vol_label();
    if (vol_label) {
        printf("Volume label: %s\n", vol_label);
        free(vol_label);
    }

    /* Get directory entries (dummy implementation) */
    bytes = nqp_getdents(fd, buffer, sizeof(buffer));
    if (bytes < 0) {
        fprintf(stderr, "nqp_getdents failed\n");
    } else {
        printf("nqp_getdents returned %zd bytes\n", bytes);
    }

    /* Process entry sets */
    es = entrysets(&se, 0);
    validate_entry_set(es);
    free(es->file);
    free(es->filenames);
    free(es);

    /* Unicode to ASCII conversion */
    ascii_str = unicode2ascii(sample_unicode, sizeof(sample_unicode)/sizeof(sample_unicode[0]));
    if (ascii_str) {
        printf("Converted string: %s\n", ascii_str);
        free(ascii_str);
    }

    /* Get next cluster in chain */
    uint32_t next_cluster = next_cluster_in_chain(&se, se.first_cluster);
    printf("Next cluster: %u\n", next_cluster);

    /* Close the file */
    if (nqp_close(fd) != 0) {
        fprintf(stderr, "nqp_close failed\n");
        return EXIT_FAILURE;
    }

    /* Unmount the filesystem */
    err = nqp_unmount();
    if (err != NQP_SUCCESS) {
        fprintf(stderr, "nqp_unmount failed with error code %d\n", err);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
  
/* End of C translation */
  

