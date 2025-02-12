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
#pragma pack(push, 1)
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

    // Initialize **everything** to zero.
    uint32_t inode_count = 0;
    uint32_t block_count = 0;
    uint16_t magic = 0;
    // Including structures, initialize with {0}.
    ext2_superblock super = {0};

    if ( argc != 2 )
    {
        fprintf( stderr, "Usage: ./ext2-inspect <ext2-volume>\n" );
        exit_code = EXIT_FAILURE;
    }
    else
    {
        fd = open( argv[1], O_RDONLY );
        
        // Jumps 1024 Bytes in - to get to the superblock
        /** Read individual properties from the superblock into variables. */
        lseek( fd, 1024 /* or 0x400*/, SEEK_SET );
        // Study the formatting of the SuperBlock cuz we are just fetching data from the formating
        read( fd, &inode_count, 4 ); // read the first four bytes - which represent the I node count. 
        read( fd, &block_count, sizeof(uint32_t) );  
        
        lseek( fd, 48, SEEK_CUR ); // <-- this is useful when iterating over
                                   //     tables of structures (e.g., inodes
                                   //     in the inode table: seek to the
                                   //     beginning of the inode table, then
                                   //     seek with SEEK_CUR to the nth inode).
        read( fd, &magic, 2 );

        printf("# of inodes: %d\n", inode_count );
        printf("# of blocks: %d\n", block_count );
        printf("Magic: 0x%x\n", magic );

        /** Read an entire struct in one go. */
        lseek( fd, 0x400, SEEK_SET );
        read( fd, &super, sizeof( ext2_superblock) );

        printf("# of inodes: %d\n", super.inodes_count );
        printf("# of blocks: %d\n", super.blocks_count );
        printf("Magic: 0x%x\n", super.magic );
    }

    return exit_code;
}


