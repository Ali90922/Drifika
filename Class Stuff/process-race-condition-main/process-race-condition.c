#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <sys/wait.h>
#include <fcntl.h>

int main(void)
{
    char line[20] = {0};
    int fd = open("exfat.txt", O_RDONLY);
    int state = 0;
    int file_names = 0;
    pid_t child = 0;

    child = fork();

    while( read( fd, line, 20 ) != 0 )
    {
        line[strlen(line)-1] = '\0';
        printf("[%d] %s\n", getpid(), line );
        if ( strncmp( line, "FILE.", 5 ) == 0 )
        {
            assert( state == 0 ); // we should be starting a new
                                  // entry set.
            assert( file_names == 0 ); // we should not have any
                                       // file names left to read
            state = 1;
        }
        else if ( strncmp( line, "STREAM EXTENSION", 16 ) == 0 )
        {
            assert( state == 1 ); // we should have already read
                                  // a file entry.
            assert( file_names == 0 ); // we should not have any
                                       // file names left to read
            state = 2;
            file_names = atoi( &line[16] );
        }
        else if ( strncmp( line, "FILE NAME", 9 ) == 0 )
        {
            assert( state == 2 );
            assert( file_names > 0 ); // there should still be
                                      // file names left to read.

            file_names--;
        }

        if ( state == 2 && file_names == 0 )
        {
            state = 0; // if we have no file names left, then
                       // we are back at the start state.
        }
    }

    if ( child != 0 )
    {
        wait( NULL );
    }

    return EXIT_SUCCESS;
}
