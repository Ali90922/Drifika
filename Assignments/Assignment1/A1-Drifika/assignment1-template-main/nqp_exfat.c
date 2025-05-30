#include <assert.h>
#include <stdlib.h>

#include "nqp_io.h"
#include "nqp_exfat_types.h"

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
char *unicode2ascii( uint16_t *unicode_string, uint8_t length )
{
    assert( unicode_string != NULL );
    assert( length > 0 );

    char *ascii_string = NULL;

    if ( unicode_string != NULL && length > 0 )
    {
        // +1 for a NULL terminator
        ascii_string = calloc( sizeof(char), length + 1); 

        if ( ascii_string )
        {
            // strip the top 8 bits from every character in the 
            // unicode string
            for ( uint8_t i = 0 ; i < length; i++ )
            {
                ascii_string[i] = (char) unicode_string[i];
            }
            // stick a null terminator at the end of the string.
            ascii_string[length] = '\0';
        }
    }

    return ascii_string;
}

nqp_error nqp_mount( const char *source, nqp_fs_type fs_type )
{
    (void) source;
    (void) fs_type;

    return NQP_INVAL;
}

nqp_error nqp_unmount( void )
{
    return NQP_INVAL;
}

int nqp_open( const char *pathname )
{
    (void) pathname;

    return NQP_INVAL;
}

int nqp_close( int fd )
{
    (void) fd;

    return NQP_INVAL;
}

ssize_t nqp_read( int fd, void *buffer, size_t count )
{
    (void) fd;
    (void) buffer;
    (void) count;
    
    return NQP_INVAL;
}
