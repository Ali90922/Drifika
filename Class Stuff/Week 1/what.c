#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

// Some sample code to help us think about what an OS might be doing, and what
// an OS might be tracking on our behalf.

int main(void)
{
    int fd;
    ssize_t bytes_read;
    
    char *buffer;

    // 1. What are we asking the OS to do here?
    // 2. What does the OS need to keep track of for it to do this for us?
    fd = open( "what.c", O_RDONLY );

    if ( fd != -1 )
    {
        // 1. What are we asking the OS to do here?
        // 2. What does the OS need to keep track of for it to do this for us?
        buffer = malloc(sizeof(char) * 10);

        if ( buffer != NULL )
        {
            // 1. What are we asking the OS to do here?
            // 2. What does the OS need to keep track of for it to do this for us?
            bytes_read = read( fd, buffer, 10 );
        }

        // 1. What are we asking the OS to do here?
        // 2. What does the OS need to keep track of for it to do this for us?
        // go back to the beginning of the file:
        lseek( fd, 0, SEEK_SET );
    }

    // oops, I forgot to call free();

    // 1. What are we asking the OS to do when we exit or return from the main
    //    function?
    // 2. What does the OS need to keep track of for it to do this for us?
    return EXIT_SUCCESS;
}

// COMP 3430 Operating Systems Winter 2025
// (c) Franklin Bristow
