#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    printf("Going to start calling malloc.\n");
    for (int i = 0; i < 10000000; i++)
    {
        int *ptr = malloc(sizeof(int));
        (*ptr)++;
        printf("%p\n", (void*)ptr);
    }
    getchar();
    return EXIT_SUCCESS;
}
