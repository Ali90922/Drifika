#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>

void handle_signal(int sig)
{
    // wat
    printf("Handling signal %d\n", sig);
}

int main(void)
{
    int *something = NULL;

    // signal(SIGSEGV, handle_signal);

    // Expecting something like: "The value of something is NULL null (nil)
    // (null)" or some variation thereof.
    printf("The value of something is %p\n", (void *) something);
    printf("oops.\n");
    printf("The value *at* something is %d\n", *something );

    return EXIT_SUCCESS; // let's be optimistic
}
