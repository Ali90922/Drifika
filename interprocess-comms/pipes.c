#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/wait.h>
#include <stdint.h>
#include <fcntl.h>

char **split(char *string);

int main(int argc, char *argv[], char *envp[])
{

    (void) envp;
    if (argc < 2)
    {
        fprintf(stderr, "%s requires at least one argument.\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char **cmd_args;
    pid_t child_pid;
    int all_pipes[2];
    uint8_t byte;
    
    (void) cmd_args;

    // not *strictly* required, but here for cases where the readers
    // close the read end of the pipe and we've got nothing left to
    // write() to (when we `write` to a pipe who's read end is closed,
    // we get sent a signal to stop, the default action for SIGPIPE is
    // to exit).
    signal(SIGPIPE, SIG_IGN);

    printf("%d is about to start [%s]\n", getpid(), argv[1]);
    cmd_args = split(argv[1]);

    // Here's where we should do some piping stuff!
    // --------------------------------------------
    // cause two new entries to be added to our open file
    // table: one read end of a pipe and one write end
    // of a pipe.

    pipe( all_pipes );

    // duplicate our PCB, including the open file table
    // and the OFT includes both ends of the pipe.
    child_pid = fork();

    if ( child_pid == 0 )
    {
        // in the child
        
        // close the write end of the pipe.
        close( all_pipes[1] );

        // manipulate my open file table, and I want
        // to replace the entry for STDIN
        dup2( all_pipes[0], STDIN_FILENO );
        // now read()-ing from STDIN_FILENO will actually
        // read() on the read of the pipe.
        execvp( cmd_args[0], cmd_args );

        // we have lost control because we just replaced
        // our code section with someone else's code.
        fprintf(stderr, "Failed to exec.\n");
        exit(EXIT_FAILURE);
    }

    // in the parent process, parent process will never
    // read() from the pipe.
    close( all_pipes[0] ); 

    while( read( STDIN_FILENO, &byte, 1 ) > 0 )
    {
        // write the byte we got from stdin to the
        // write end of the pipe.
        write( all_pipes[1], &byte, 1 );
    }

    close( all_pipes[1] ); // close the write end of the pipe
                           // this says "no more data left"
                           // anyone read()ing the read end
                           // of the pipe will have their read()
                           // return and it will have a value
                           // of zero.
    wait( NULL );
    // --------------------------------------------

    printf("All done!\n");

    return EXIT_SUCCESS;
}

char **split(char *string)
{   
    char **words;
    char *word;
    char *copy;
    int spaces = 0;

    copy = strdup(string);
    
    // we're going to split on whitespace, count spaces
    // ASSUMPTION: strings are formatted with explicitly one space in between each token.
    for (size_t i = 0; i < strlen(string); i++)
    {
        if (isspace(string[i]))
        {
            spaces++;
        }
    }

    words = malloc(sizeof(char*) * (spaces + 2));
    for (int i = 0; i < spaces + 1; i++)
    {
        word = strtok(copy, " ");
        words[i] = malloc(sizeof(char) * (strlen(word) + 1));
        strcpy(words[i], word);
        copy = NULL;
    }

    words[spaces + 1] = NULL;
    return words;
}
