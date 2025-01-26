#include <assert.h>
#include <stdlib.h>

#include "nqp_io.h"
#include "nqp_exfat_types.h"

// My Task is to :
// Implement the functionality for the five functions in nqp_io.h.
// Ensure they correctly handle the exFAT file system format as described in the  nqp_exfat_types.h header file

/**
 * Convert a Unicode-formatted string containing only ASCII characters
 * into a regular ASCII-formatted string (16 bit chars to 8 bit
 * chars).
 *
 * NOTE: this function does a heap allocation for the string it
 *       returns (like strdup), caller is responsible for `free`-ing the
 *       allocation when necessary.
 *
 * uint16_t *unicode_string: the Unicode-formatted string to be
 *                           converted.
 * uint8_t   length: the length of the Unicode-formatted string (in
 *                   characters).
 *
 * returns: a heap allocated ASCII-formatted string.
 */
char *unicode2ascii(uint16_t *unicode_string, uint8_t length)
{
    assert(unicode_string != NULL);
    assert(length > 0);

    char *ascii_string = NULL;

    if (unicode_string != NULL && length > 0)
    {
        // +1 for a NULL terminator
        ascii_string = calloc(sizeof(char), length + 1);

        if (ascii_string)
        {
            // strip the top 8 bits from every character in the
            // unicode string
            for (uint8_t i = 0; i < length; i++)
            {
                ascii_string[i] = (char)unicode_string[i];
            }
            // stick a null terminator at the end of the string.
            ascii_string[length] = '\0';
        }
    }

    return ascii_string;
}

// My job starts from here :

/**
 * "Mount" a file system.
 *
 * This function must be called before interacting with any other nqp_*
 * functions (they will all use the "mounted" file system).
 *
 * This function does a basic file system check on the super block of the file
 * system being mounted.
 *
 * Parameters:
 *  * source: The file containing the file system to mount. Must not be NULL.
 *  * fs_type: The type of the file system. Must be a value from nqp_fs_type.
 * Return: NQP_UNSUPPORTED_FS if the current implementation does not support
 *         the file system specified, NQP_FSCK_FAIL if the super block does not
 *         pass the basic file system check, NQP_INVAL if an invalid argument
 *         has been passed (e.g., NULL),or NQP_OK on success.
 */
nqp_error nqp_mount(const char *source, nqp_fs_type fs_type)
{

    // Validate Parameters :
    if (source == NULL || fs_type != NQP_FS_EXFAT)
    {
        return NQP_INVAL;
    }

    (void)source;
    (void)fs_type;

    return NQP_INVAL;
}

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
    return NQP_INVAL;
}

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
    (void)pathname;

    return NQP_INVAL;
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
    (void)fd;

    return NQP_INVAL;
}

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
    (void)fd;
    (void)buffer;
    (void)count;

    return NQP_INVAL;
}
