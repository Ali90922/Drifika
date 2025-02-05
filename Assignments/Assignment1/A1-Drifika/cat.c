#include "nqp_io.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    int fd = 0;
    ssize_t bytes_read = 0;
    char buffer[256] = {0};
    int exit_code = EXIT_SUCCESS;
    nqp_error err = nqp_mount(argv[1], NQP_FS_EXFAT);

    if (err == NQP_OK)
    {
        for (int i = 2; i < argc; i++)
        {
            fd = nqp_open(argv[i]);

            if (fd == NQP_FILE_NOT_FOUND)
            {
                fprintf(stderr, "%s not found\n", argv[i]);
                exit_code = EXIT_FAILURE;
            }
            else
            {
                // Fix this a bit -- Need to input the Exact Size
                // Need to get the File size using the stream extension first.

                // int FileeeeSizeeee = nqp_size(fd);
                // printf("File size: %d bytes\n", FileeeeSizeeee);

                bytes_read = nqp_read(fd, buffer, 2157);

                for (ssize_t i = 0; i < bytes_read; i++)
                {
                    putchar(buffer[i]);
                }

                nqp_close(fd);
            }
        }

        nqp_unmount();
    }

    return exit_code;
}
