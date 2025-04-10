#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    char buffer = '\0';

    printf( "Enter a single character, then press Enter or Ctrl+D: " );

    // causes my process to change states from running
    // to blocked.
    buffer = getchar();

    // after this line returns (getchar()), the process is back in
    // a running state.

    putchar( buffer );

    return EXIT_SUCCESS;
}
