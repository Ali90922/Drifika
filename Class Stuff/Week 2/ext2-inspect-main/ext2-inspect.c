#include <stdio.h>
#include <stdlib.h>
#include <stdint.h> // <<<<< this is new, and important!
#include <fcntl.h>
#include <unistd.h>

/**
 * The #pragmas are necessary for our structure so that we can read(2) directly
 * into an instance of this structure. If we don't put the #pragma, the compiler
 * is free to reorder and/or place padding in between the members of the struct
 * (padding might be added to align parts of or the whole structure to memory
 * boundaries).
 *
 * You can read more about #pragma pack here:
 * https://learn.microsoft.com/en-us/cpp/preprocessor/pack?view=msvc-170
 */
#pragma pack(1)
#pragma pack(push)
typedef struct EXT2_SUPERBLOCK
{
    uint32_t inodes_count;
    uint32_t blocks_count;
    uint8_t  skip[48]; // "skip" 48 bytes; there's actual stuff here
                       // but it's stuff we don't care about right now
                       // (e.g., s_free_blocks_count).
    uint16_t magic;
   
} ext2_superblock;
#pragma pack(pop)

int main( int argc, char **argv )
{
    int exit_code = EXIT_SUCCESS;
    int fd = -1;
    uint32_t inode_count = 0;
    uint32_t block_count = 0;
    if ( argc != 2 )
    {
        fprintf( stderr, "Usage: ./ext2-inspect <ext2-volume>\n" );
        exit_code = EXIT_FAILURE;
    }
    else
    {
        fd = open( argv[1], O_RDONLY );

        // to get to the supetblock, we have to go 10 
        // This is the same as 0x4000 
        lseek(fd, 0x400,SEEK_SET);

        read(fd, &inode_count,4);
        printf("# of inodes: %d\n", inode_count
        
    }

    return exit_code;
}


