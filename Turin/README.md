# NQP Shell - COMP 3430 Assignment 2

## Description
This project implements an interactive shell (`nqp_shell`) for an exFAT-formatted volume, using the Not Quite POSIX (NQP) API. The shell supports built-in commands, launching processes, input redirection, pipes, and logging.

## Compilation
To compile the shell, run:


```bash
   make
   ```

To start the shell with a volume 

```bash
   ./nqp_shell root.img
   ```


To Enable Logging 

```bash
   ./nqp_shell root.img -o log.txt
```


Features

### Built-in Commands
cd directory - Change the current working directory. <br>
pwd - Print the current working directory. <br>
ls - List the contents of the current directory.<br>
clear - Clears the terminal screen.<br>

###Process Execution<br>
Runs programs stored in the provided volume.
Uses memfd_create and fexecve to execute programs from the volume.
Input Redirection
Redirects input from a file using < filename.
Reads the file into a memory-backed file (memfd_create).
Sets up input redirection using dup2.
Piping
Supports multiple pipes (e.g., cat file.txt | grep hi | sort).
Connects processes using pipe and dup2.
Logging
Duplicates shell output to log.txt when run with the -o option.
Intercepts and logs the final process output in a pipeline.

