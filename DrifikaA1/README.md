# COMP 3430 Assignment 1: exFAT File System Implementation

**Author:** Ali Nawaz  
**Course:** COMP 3430  
**Term:** Winter 2025

## Overview

This project implements a read-only exFAT file system interface that mimics many aspects of the POSIX file system API. The goal of the assignment is to support basic file system operations on an exFAT-formatted disk image. The implementation includes functions to mount and unmount the file system, open files, read file data, list directory entries, and retrieve file sizes. In addition, a simple open file table (OFT) has been implemented to allow multiple files to be accessed simultaneously.

The interface defined in the header file `nqp_io.h` is used throughout the code, while exFAT-specific types and constants are provided in `nqp_exfat_types.h`. A main interface (entry point) has been created to demonstrate and test the implemented functions.

## Features

### Mounting & Unmounting

- **nqp_mount:** Opens the file system image, reads the Main Boot Record (MBR), and performs a series of checks to ensure the file system is consistent. Critical fields such as the file system name (`"EXFAT   "`), boot signature (`0xAA55`), the `must_be_zero` block, and `first_cluster_of_root_directory` are validated.
- **nqp_unmount:** Closes the file system image and resets the mounted state.

### File Operations

- **nqp_open:** Searches the directory structure (using a tokenized path) to locate and open a file. The file descriptor returned is the first cluster number of the file, and the file is recorded in an open file table.
- **nqp_read:** Reads data from an open file by following its FAT chain. This function properly handles cases where file data spans multiple clusters.
- **nqp_close:** Marks a file as closed (currently a placeholder since the open file table is simple).
- **nqp_size:** Retrieves the size of a file by scanning directory entries for a matching first cluster.

### Directory Operations

- **nqp_getdents:** Reads directory entries one at a time. This function uses static variables to track progress through a directory, converts Unicode filenames to ASCII using `unicode2ascii`, and handles multi-cluster directories.

### Utilities

- **unicode2ascii:** Converts 16-bit Unicode strings (with only ASCII characters) to regular 8-bit C strings.
- **print_open_file_table:** (For debugging) Prints the current status of the open file table.
- **Main Interface:** A `main` interface has been created that demonstrates the functionality of the file system API. This interface is used by utility programs such as `cat`, `ls`, and `paste` to interact with the file system image.

## Compilation

The project is designed to compile on the Aviary system. To compile the project, simply run:

```bash
make
```
To clean previous builds, run:

```bash
make clean
```

### Running the Program :
After compiling, you can run the provided utility programs that use the implemented exFAT interface. For example

cat: Display the contents of a file.
```bash
./cat Drifika.img README.md
```

ls : List the contents of a directory.
```bash
./ls Drifika.img Juventus
```

paste : Merge or display contents from multiple files
```bash
./paste Drifika.img README.md Uefa.txt
```

Please Ensure the file sstem image(Drifika.img) and the target files or directories exist and are in the same directory as the program. 



I have also implemeted a main interface. This interface provides a simplified, read-only API for interacting with exFAT-formatted disk images, similar to common POSIX file system calls. It includes functions for mounting (nqp_mount) and unmounting (nqp_unmount) the file system, opening files (nqp_open), reading file data (nqp_read), listing directory entries (nqp_getdents), and retrieving file sizes (nqp_size). To handle the unique aspects of exFAT—such as 16-bit Unicode filenames and cluster-based data storage—the interface incorporates utility functions like unicode2ascii and maintains a minimal open file table for managing multiple file accesses simultaneously. This design enables utility programs like cat, ls, and paste to interact with the file system image in an efficient and straightforward manner.

Below is a render of my interface : 

Welcome to exFAT CLI Tester

Commands:
  mount <fs_image>        - Mount an exFAT file system
  open <filename>         - Open a file or directory
  read <fd> <size>        - Read bytes from an open file
  getdents <fd> <dummy>   - List directory entries (reads one entry at a time)
  close <fd>              - Close an open file
  unmount                 - Unmount the file system
  exit                    - Exit the program


it can be accessed after running the following after the make command :
```bash
./main
```





