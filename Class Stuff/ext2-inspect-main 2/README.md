---
title: COMP 3430 Operating Systems
subtitle: Inspecting ext2
author: Franklin Bristow
date: Winter 2025
toccolor: blue
toc: true
---

Overview
========

This contains [some instructions for creating an ext2-formatted volume] plus
some initial code that we're going to be using to inspect the volume that we've
created.

[some instructions for creating an ext2-formatted volume]: #creating-a-volume

Creating a volume
=================

Before we can take a look at an ext2-formatted volume, we need to *create* an
ext2-formatted volume. Creating a volume (on Linux!) is straightforward:

```bash
# Create an empty file of a certain size using `truncate`:
truncate --size 256k ext2-volume

# Then format the volume as an ext2 file system:
mkfs.ext2 -d ext2-files/ -L comp3430-w25 -I 128 ext2-volume
```

These two commands will:

1. Create an empty file that's 256KB in size.
2. Format that empty file with an ext2 file system with the following
   properties:
   * The files in the directory `ext2-files/` will be put into the root
     directory (`/`) of the ext2 volume (this is the `-d` option).
   * The ext2 super block will have a label of `comp3430-w25` (this is the `-L`
     option).
   * The inodes in this volume will be 128 bytes in size (this is the `-I`
     option; the default is 256 bytes; take a look at the warning that
     `mkfs.ext2` prints when we specify that the inode size is 128 bytes!).

You should take a quick look at the file(s) in the directory `ext2-files/`
beside this `README.md` file just to get a sense of what the content of these
files is.

You may want to refer to the manual pages for the commands to get a more
comprehensive description of their options.

Inspecting the volume
=====================

We're going to start working with this volume on the command line using a
program called `hexdump`. `hexdump`... [does exactly what it says on the tin]:
it will print out the contents of a file as hexadecimal values.

We're going to start by running:

```bash
hexdump -C -v ext2-volume | less
```

This says:

1. Print out the file in "canonical" form (this is the `-C` option). This is
   a display option for how `hexdump` will print out the bytes.
2. Don't skip printing out long sections of zeros (this is the `-v` option).
   There are long stretches of `0x00` in an ext2-formatted volume, and at least
   for now I want us to be able to see them.
3. Pipe the output to `less`: printing out 256,000 bytes (even if there are 16
   bytes per line) will quickly fill our terminal. `less` will let us page
   through and search for strings.

In class we will be working with [Dave Poirier's ext2 documentation] to inspect
this volume.

[does exactly what it says on the tin]:
https://en.wikipedia.org/wiki/Does_exactly_what_it_says_on_the_tin
[Dave Poirier's ext2 documentation]: https://www.nongnu.org/ext2-doc/ext2.html

Building and running
====================

The file `ext2-inspect.c` is a skeleton C program so that we don't have to start
from nothing; it's currently compilable, but is not functional.

You can build the program using `make`:

```bash
make
```

You can run this program on the command line. The program takes only one
argument: an ext2-formatted volume:

```bash
./ext2-inspect ext2-volume
```
