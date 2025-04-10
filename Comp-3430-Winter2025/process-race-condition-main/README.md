---
title: COMP 3430 Operating Systems
subtitle: Race conditions and processes
author: Franklin Bristow
date: Winter 2025
toccolor: blue
toc: true
---

Overview
========

We talk a lot in this course about race conditions and threads, but this is an
example of how we can have a race condition with *processes*.

Building and running
====================

This repository has one file: `process-race-condition.c`

You can build the program with `make`:

```bash
make
```

`process-race-condition.c`
--------------------------

As in the repository, this program will (probably!) crash when you run it with
some failing `assert`s:

```bash
./process-race-condition
```

Output:

```
[589560] FILE...............

[589560] STREAM EXTENSION 1.

[589560] FILE...............
[589561] FILE NAME..........

process-race-condition: process-race-condition.c:25: int main(void): Assertion `state == 0' failed.
process-race-condition: process-race-condition.c:42: int main(void): Assertion `state == 2' failed.
fish: Job 1, './process-race-condition' terminated by signal SIGABRT (Abort)
```

Notice that there are *two* failing `assert`ions: one in the main process and
one in the child process. Both fail because the state machine that's working
through the "directory entries" in our "exFAT volume" is getting into an
unexpected state: The first process (589560 in this case) reads a `FILE` then
`STREAM EXTENSION` (as expected), then its next record is a `FILE` (which is
unexpected). This is happening because the second process (589561 in this case)
is using the same file descriptor and the same entry in the system open file
table, so when it enters the loop it reads the `FILE NAME` that the first
process was expecting to read. The second process fails now because it's in an
unexpected state (it shouldn't see any `FILE NAME` until after it's in state 2;
state 2 implies that it has read a `STREAM EXTENSION`).

This code works as expected if you comment out the line where it calls `fork`
(only one process is working with the file descriptor, so we don't have a race
condition on the open file table entry).

This code will also work if you move the `open` system call to after calling
`fork` (each process gets its own entry in the open file table so they can have
different offsets in the same file).

The shared resource between these two processes is the system open file table,
and the race condition is happening because they are both trying to modify it
when they do things like call `read`!
