---
title: COMP 3430 Operating Systems
subtitle: Virtual memory
author: Franklin Bristow
date: Winter 2025
---

Overview
========

While none of this code implements virtual memory, it reveals to us a little bit
about how address spaces are organized and reveals to us a little bit about how
our OS *doesn't* directly manage the contents of the heap. At least, not at the
same level of granularity we thought it might.

Building and running
--------------------

This repository has two code files: `addresses.c` and `allocate.c`.

You can build these programs using `make`:

```bash
make
```

### `addresses.c`

`addresses.c` in and of itself doesn't do much. You can run it without any
arguments:

```bash
./addresses
```

The addresses that it prints out are where the different segments of memory
(code, heap, stack) approximately are.

The code blocks on `getchar()` so that you can inspect the `/proc` file system
for the process. Open up the file `/proc/$PID/maps` (use the PID that was
printed by `addresses`) and you can see the actual locations and addresses for
these segments.

Try running this a few times. The addresses change! Why? 

To help decoding `/proc/$PID/maps`, you should take a look at `man procfs`. You
should also try taking a look at the ELF headers for this program using
something like `readelf`. Are you able to find the code section in the ELF file
and the code segment in the memory maps for this program?

### `segmentation-fault.c`

[Does exactly what it says on the tin]. You can run this code on the command
line:

```
./segmentation-fault
```

~~... but it doesn't currently (before class!) seg fault. We're going to
successfully make it fail in class. For now it fails to fail, successfully.~~

And now it successfully fails in the way we were expecting it to.

We tried to register a signal handler, and we can register a signal handler,
but... y tho? And what the heck is going on when the handler is actually
running?!

[Does exactly what it says on the tin]: https://en.wikipedia.org/wiki/Does_exactly_what_it_says_on_the_tin

### `allocate.c`

`allocate.c` in and of itself doesn't do  much. You can run it without any
arguments:

```bash
./allocate
```

All this does is print out addresses that were allocated with `malloc()`, then
terminates.

We want to use this program to see if the OS is allocating memory in the heap
for us, so run this program with `strace`:

```bash
strace -e trace=%memory ./allocate > /dev/null
```

Try doing that and changing how many times `allocate.c` loops. Does `malloc`
appear in the system call trace? Does *anything* appear in the system call trace
the same number of times you're calling `malloc`?

The `> /dev/null` bit redirects all output from `allocate` into a special device
called `/dev/null`. You can think of this as a trash can for output: any
`write()`s to `/dev/null` just get ignored!
