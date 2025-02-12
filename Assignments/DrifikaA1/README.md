# COMP 3430 Assignment 1: exFAT File System Implementation

**Author:** Your Name  
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


