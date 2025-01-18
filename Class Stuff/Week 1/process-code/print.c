#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/**
 * Print out how the program should be run, then exit indicating failure.
 *
 * Parameters:
 *  * process_name: the name of the program when it was launched.
 */
void exit_usage( char *process_name )
{
    fprintf( stderr, "Usage: %s -c <letter>\n", process_name );
    exit( EXIT_FAILURE );
}

int main( int argc, char *argv[] )
{
    // 0 is the "null byte"; we can't type that in on the keyboard, so
    // it's a good sentinel value to indicate that nothing was typed in.
    char letter = 0;
    int opt;

    // This is an example from the manual page for getopt (`man 3 getopt`).
    while ( ( opt = getopt(argc, argv, "c:") ) != -1 )
    {
        switch ( opt )
        {
            case 'c':
                letter = *optarg;
                break;
        }
    }

    // exit before doing anything if the program wasn't run correctly.
    //
    if ( letter == 0 )
    {
        exit_usage( argv[0] );
    }

    // print the letter and sleep for 1 second, forever.
    while( 1 )
    {
        putchar( letter );
        putchar( '\n' );
        sleep( 1 );
    }

    return EXIT_SUCCESS;
}
