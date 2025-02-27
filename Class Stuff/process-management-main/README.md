---
title: COMP 3430 Operating Systems
subtitle: Process management
author: Franklin Bristow
date: Winter 2025
toccolor: blue
toc: true
---

Overview
========

This contains some code we will be using to *begin* thinking about process
management and lifecycle (this is `basic-io.c`) and then some code we will be
using to *begin* managing processes (this is `process-management.c`).

Building and running
====================

This repository has two files: `basic-io.c` and `process-management.c`.

You can build both programs using `make`:

```bash
make
```

`basic-io.c`
------------

This program doesn't actually do very much, but it gives us an opportunity to
think about two things:

1. The state of our process (it goes from running, to blocked, back to running,
   then exits).
2. How our program moves from "user space" to "kernel space" when we're asking
   for stuff to happen in the form of system calls.

You can run this program on the command line:

```bash
./basic-io
```

In general: our program is "blocked" when it's halted and waiting for something
to happen. In this program right now, the process will "block" on line 12 where
we call `getchar()`. We can inspect the state of this program using the program
`ps` on the command line. We have to pass the `a` option to have `ps` print all
processes:

```bash
ps a
```

On Linux, the state of this process is `S+` (it's "interruptible sleeping"). For
us and in general, the state of the process is "blocked".

We can look at how our program moves between user space and kernel space or
operating system space in terms of mode switching by using the program `strace`.

::: note

`strace` is only available on Linux. If you're using macOS (or if you don't have
the Windows Subsystem for Linux installed), you may want to run this example on
Aviary.

:::

You can run this program with `strace` to see how it's actually making system
calls (and mode switching between the process and the operating system):

```bash
strace ./basic-io
```

`process-management.c`
----------------------

This program gives us a chance to see how a process can ask for another process
to be created using the `fork` system call, and how to ask for the code segment
for a process to be replaced using the `exec` family of calls (specifically in
our case `execvp`).

This program also introduces the idea of synchronization using the `wait` family
of system calls, specifically in our case `waitpid`. The parent process in this
example will not proceed to continue doing work until after the child process
has fully exited.

You can run this program on the command line:

```bash
./process-management
```

As published, this will only block once *per process* (so twice in total)
after the call to `fork` (once for the `getchar()` in the child process, once
for the `waitpid` in the parent process).

After we call `fork`, you can see that there are two processes running using the
`ps` program (again, `ps a`).

You can press Enter on your keyboard to unblock the processes. At this point the
child process will unblock (it was blocked on `getchar()`), ask the OS to
replace its code segment with the code segment for `cat`, `cat` will then print
out the contents of `process-management.c`, and then exit. Upon exiting, the
parent process will unblock from `waitpid`, print some stuff, then finally exit.

Notice how we're declaring a stack allocated variable (`letter`), assigning it
the value of `w` in the initial process. In the child process (the process where
`0` is returned from `fork`) we change the value of `letter` to `c`, but this
*only* affects the child process.

Notice further that now that we're calling `waitpid`, we can **guarantee** the
order of output from this program where we were not able to guarantee the order
of output from the program when we just allowed both processes to run to
completion without blocking (or even with blocking with `getchar()`).
