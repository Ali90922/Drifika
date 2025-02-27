#include <stdio.h>  
// OS Detail:
// - When the process is loaded, the OS maps shared libraries (like libc) into its address space.
// - The dynamic linker resolves symbols for stdio functions (e.g., printf, getchar, putchar).
// - The OS allocates and initializes the process's file descriptor table:
//     * File Descriptor 0: Standard Input (stdin)
//     * File Descriptor 1: Standard Output (stdout)
//     * File Descriptor 2: Standard Error (stderr)

#include <stdlib.h> 
// OS Detail:
// - Similarly, the OS maps the stdlib library into the process's memory.
// - Functions and constants (e.g., EXIT_SUCCESS) are resolved by the dynamic linker.
// - Memory for the code and data segments of this library is allocated as part of the process’s address space.

int main(void)    
// OS Detail:
// - The OS sets the entry point for execution (the Program Counter, PC, is initialized to the address of main).
// - A stack is allocated for the process, and the Stack Pointer (SP) is set to the top of this stack.
// - The process control block (PCB) is created/updated with initial context (registers, PC, SP, scheduling information, etc.).
{
    char buffer = '\0';  
    // OS Detail:
    // - The variable 'buffer' is allocated on the process's stack.
    // - The OS keeps track of the stack’s memory region and the current Stack Pointer (SP) in the PCB.
    // - This allocation is part of the process’s local memory for function calls.

    printf("Enter a single character, then press Enter or Ctrl+D: ");
    // OS Detail:
    // - The call to printf() eventually invokes a write system call.
    // - The OS uses the process's file descriptor table to locate stdout (File Descriptor 1).
    // - The OS verifies that stdout is properly connected to a terminal or output device.
    // - The output string is buffered and written to the device; the OS may interact with device drivers to handle this output.

    // causes my process to change states from running to blocked.
    buffer = getchar();
    // OS Detail:
    // - getchar() attempts to read from stdin (File Descriptor 0).
    // - If no input is available, the OS puts the process into a blocked (or waiting) state.
    // - The OS updates the PCB to reflect this state change, removing the process from the CPU scheduler's ready queue.
    // - The OS monitors the stdin file descriptor (possibly via mechanisms like select() or poll()).
    // - Once input is available (e.g., when the user types a character and presses Enter or signals EOF with Ctrl+D),
    //   the OS marks the process as ready, restores its context (including the PC and SP), and reschedules it for execution.

    // after this line returns (getchar()), the process is back in a running state.
    putchar(buffer);
    // OS Detail:
    // - putchar() writes the character stored in 'buffer' to stdout (File Descriptor 1).
    // - The stdio library uses a write system call to send the character to the output device.
    // - The OS checks that stdout is open and valid, then passes the data to the terminal’s device driver.
    // - Buffering or low-level I/O operations are handled by the OS, ensuring data is displayed properly.

    return EXIT_SUCCESS;
    // OS Detail:
    // - Returning from main indicates that the process has finished executing.
    // - The OS updates the PCB to mark the process as terminated.
    // - It then performs cleanup tasks: deallocating the stack and heap memory, closing open file descriptors,
    //   and removing the process from the scheduler.
    // - The termination status (EXIT_SUCCESS) may be passed to the parent process or recorded by the OS.
}
