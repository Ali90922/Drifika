#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>

int main(void)
{
    // the address of a function shoud be in
    // the code section of an address space
    void *main_addr = (void*) &main;

    // the address of a stack-allocated var
    // should be in the stack section of
    // an address space
    void *stack_ptr = (void*) &main_addr;

    // memory allocated with malloc should be
    // in the heap section of an address space
    void *heap_ptr = malloc(1);

    // print out the pid so we can inspect
    // the state of this process
    printf("My pid is: %d\n", getpid());

    printf("Main is at %p\n", main_addr);
    printf("Stack is at %p\n", stack_ptr);
    printf("Heap is at %p\n", heap_ptr);

    // confirm our assumptions about the layout
    // of the address space
    assert( main_addr < heap_ptr );
    assert( heap_ptr < stack_ptr );

    // block so we can inspect the state
    // of this process
    printf("Press enter to exit.\n");
    getchar();

    return EXIT_SUCCESS;
}
