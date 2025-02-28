#include <stdio.h>
#include <stdlib.h>
#include "nqp_io.h"
#include <string.h>


// Tasks : 
//  1. Implement ls 

int main( int argc, char *argv[], char *envp[] )
{
#define MAX_LINE_SIZE 256
    char line_buffer[MAX_LINE_SIZE] = {0};
    char *volume_label = NULL;
    nqp_error mount_error;

    (void) envp;

    if ( argc != 2 )
    {
        fprintf( stderr, "Usage: ./nqp_shell volume.img\n" );
        exit( EXIT_FAILURE );
    }

   // Mount exFAT filesystem
    mount_error = nqp_mount( argv[1], NQP_FS_EXFAT );

    if ( mount_error != NQP_OK )
    {
        if ( mount_error == NQP_FSCK_FAIL )
        {
            fprintf( stderr, "%s is inconsistent, not mounting.\n", argv[1] );
        }

        exit( EXIT_FAILURE );
    }

    volume_label = nqp_vol_label( );

    printf( "%s:\\> ", volume_label );
    // Shell loop
    while (1) {
        printf("%s:\\> ", volume_label); // Display prompt
        if (fgets(line_buffer, MAX_LINE_SIZE, stdin) == NULL) {
            printf("\nExiting shell...\n");
            break;
        }
    line_buffer[strcspn(line_buffer, "")] = '\0';
}
    return EXIT_SUCCESS;
}
