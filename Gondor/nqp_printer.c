#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include "nqp_thread.h"
#include "nqp_thread_sched.h"

void printer( void *arg )
{
    assert( arg != NULL );

    uint64_t counter = 0;
    char letter = '\0';

    if ( arg != NULL )
    {
        letter = *(char *)arg;
        while ( counter < 200 )
        {
            printf("%c %lu\n", letter, counter );
            counter++;

            if ( counter % 50 == 0 )
            {
                nqp_yield( );
            }
        }
    }

    nqp_exit( );
}

int main( void )
{
    char *letters = "ab";
    
    nqp_thread_t *thread_a = nqp_thread_create( printer, &letters[0] );
    nqp_thread_t *thread_b = nqp_thread_create( printer, &letters[1] );

    nqp_thread_join( thread_a );
    nqp_thread_join( thread_b );

    return EXIT_SUCCESS;
}
