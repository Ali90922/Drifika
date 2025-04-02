---
title: COMP 3430 Operating Systems
subtitle: "Assignment 3: Concurrency!"
date: Winter 2025
---

Overview
========

This directory contains the following:

* This `README.md` file (you're reading it!).
* A `Makefile` that can build the sample code.
* Several interfaces that describe how concurrency could be used
  (`nqp_thread.h`, `nqp_thread_locks.h`, and `nqp_thread_sched.h`).
* Some code that uses the concurrency as defined by these interfaces
  (`nqp_printer.c`, `nqp_refiner.c`, and `nqp_sched_tasks.c`).

Building and running
====================

You can compile all programs on the command line:

```bash
make
```

`nqp_printer.c`
---------------

You can run this program on the command line:

```bash
./nqp_printer
```

This code, as is, will crash: the default return value from `nqp_thread_create`
is `NULL`, and the code in `nqp_thread_join` has an `assert`ion that the passed
argument is not `NULL`. You must minimally implement `nqp_thread_create` for
this code to (as is) not crash.

The expected output for this program is for it to switch back and forth between
printing `a` and `b` with an integer that demonstrates state begin retained
between each call.

`nqp_refiner.c`
---------------

Refine some macrodata! You can run this program on the command line:

```bash
./nqp_refiner
```

This code, as is, will crash: the default return value from `nqp_thread_create`
is `NULL`, and the code in `nqp_thread_join` has an `assert`ion that the passed
argument is not `NULL`. You must minimally implement `nqp_thread_create` for
this code to (as is) not crash.

The expected output for this program is to display progress for many workers:

[![asciicast](https://asciinema.org/a/6Ew1SkoeW9vTDaCpLPAxszxuk.svg)](https://asciinema.org/a/6Ew1SkoeW9vTDaCpLPAxszxuk)

`nqp_sched_tasks.c`
-------------------

You can run this program on the command line:

```bash
./nqp_sched_tasks
```

This code creates several different kinds of tasks: interactive tasks (ones that
use very little CPU time before yielding), semi-interactive tasks (ones that use
a moderate amount of CPU time before yielding), and non-interactive tasks (ones
that use a lot of CPU time before yielding).

This code, as is, will crash: the default return value from `nqp_thread_create`
is `NULL`, and he code in `nqp_thread_join` has an `assert`ion that the passed
argument is not `NULL`. You must minimally implement `nqp_thread_create` for
this code to (as is) not crash.

You can run and modify this program to test out each of the different scheduling
policies that you've implemented. The expected output is different for each
policy.

### Expected output for FIFO

The output for FIFO is:

```
iiiiiiiiiiiiisssssssssssssnnnnnnnnnnnnnn
iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii
iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii
iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii
iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii
iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiissssssssss
ssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssss
ssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssss
ssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssss
ssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssss
ssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssnnnnnnnnnnnnnnnnnnnn
nnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn
nnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn
nnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn
nnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn
nnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn
All done!
```

Newlines have been added for clarity in this `README.md`; the expected output
for this is for all output from threads to be on the same line.

The output for FIFO is deterministic, it should always be exactly the same 
every time the program is run.

### Expected output for RR

The output for RR is:

```
iiiiiiiiiiiiisssssssssssssnnnnnnnnnnnnnn
iiiiiiiiiiiiisssssssssssssnnnnnnnnnnnnnniiiiiiiiiiiiisssssssssssssnnnnnnnnnnnnnn
iiiiiiiiiiiiisssssssssssssnnnnnnnnnnnnnniiiiiiiiiiiiisssssssssssssnnnnnnnnnnnnnn
iiiiiiiiiiiiisssssssssssssnnnnnnnnnnnnnniiiiiiiiiiiiisssssssssssssnnnnnnnnnnnnnn
iiiiiiiiiiiiisssssssssssssnnnnnnnnnnnnnniiiiiiiiiiiiisssssssssssssnnnnnnnnnnnnnn
iiiiiiiiiiiiisssssssssssssnnnnnnnnnnnnnniiiiiiiiiiiiisssssssssssssnnnnnnnnnnnnnn
iiiiiiiiiiiiisssssssssssssnnnnnnnnnnnnnniiiiiiiiiiiiisssssssssssssnnnnnnnnnnnnnn
iiiiiiiiiiiiisssssssssssssnnnnnnnnnnnnnniiiiiiiiiiiiisssssssssssssnnnnnnnnnnnnnn
iiiiiiiiiiiiisssssssssssssnnnnnnnnnnnnnniiiiiiiiiiiiisssssssssssssnnnnnnnnnnnnnn
iiiiiiiiiiiiisssssssssssssnnnnnnnnnnnnnniiiiiiiiiiiiisssssssssssssnnnnnnnnnnnnnn
iiiiiiiiiiiiisssssssssssssnnnnnnnnnnnnnniiiiiiiiiiiiisssssssssssssnnnnnnnnnnnnnn
iiiiiiiiiiiiisssssssssssssnnnnnnnnnnnnnniiiiiiiiiiiiisssssssssssssnnnnnnnnnnnnnn
iiiiiiiiiiiiisssssssssssssnnnnnnnnnnnnnniiiiiiiiiiiiisssssssssssssnnnnnnnnnnnnnn
iiiiiiiiiiiiisssssssssssssnnnnnnnnnnnnnniiiiiiiiiiiiisssssssssssssnnnnnnnnnnnnnn
iiiiiiiiiiiiisssssssssssssnnnnnnnnnnnnnniiiiiiiiiiiiisssssssssssssnnnnnnnnnnnnnn
iiiiiiiiiiiiisssssssssssssnnnnnnnnnnnnnniiiiiiiiiiiiisssssssssssssnnnnnnnnnnnnnn
All done!
```

Newlines have been added for clarity in this `README.md`; the expected output
for this is for all output from threads to be on the same line.

The output for RR is deterministic, it should always be exactly the same every
time the program is run.

### Expected output from MLFQ

Unlike RR and FIFO, the output from MLFQ is *not* deterministic. Our
implementation of the policy relies on real clock time to determine when we
should lower the priority of tasks and when we should boost the priority of all
tasks. While the amount of time that we're sleeping for is "deterministic" (it's
not, really, but it's close enough), the use of actual time elapsed also
includes things like how much time the thread spent on the CPU waiting for the
`write` system call to be serviced, which itself includes things like mode and
context switching, and how long the thread spent waiting for our actual OS to
schedule our process again.

I (me, Franklin) have included several examples of output from my implementation
of MLFQ to give you an approximate idea of what you could expect your output to
look like.

While the output is not deterministic, it generally has the following
properties:

* Interactive tasks tend to get access to the CPU for more time earlier on, and
  thus finish earlier,
* Semi-interactive tasks tend to get access to the CPU for more time midway
  through the set of tasks and finish in the middle, and
* Non-interactive tasks tend to not get access to the CPU until the end of the
  run.

Importantly, though: all tasks *do* get some initial access to the CPU fairly
early on (notice how all tasks are run in round-robin for the very first go
through the set of tasks), with the non-interactive tasks exhausting their time
in the top priority queue very quickly.

As above, all examples have had newlines added for clarity in this `README.md`;
the expected output is for all output from threads to be on the same line.

#### MLFQ example 1

```
iiiiiiiiiiiiisssssssssssssnnnnnnnnnnnnnn
iiiiiiiiiiiiisssssssssssssnnnnnnnnnnnnnniiiiiiiiiiiiisssssssssssssnnnnnnnnnnnnnn
iiiiiiiiiiiiisssssssssssssnnnnnnnnnnnnnniiiiiiiiiiiiisssssssssssssnnnnnnnnnnnnnn
nnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn
iiiiiiiiiiiiisssssssssssssnnnnnnnnnnnnnniiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii
iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii
iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii
iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii
iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiissssssssssssssssssssssssssssssssssssssssssss
ssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssss
sssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssnnnnnnnnnnnnn
nisssssssssssssnnnnnnnnnnnnnnisssssssssssssssssssssssssssssssssssssssssssnnnnnnn
nnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn
nnnnnnnnnnnnnnnniiiiiiisssssssssssssssssssssssssssnnnnnnnnnnnnnnssssssssssnnnnnn
nnnnnnnnssssssssnnnnnnnnnnnnnnssssssssnnnnnnnnnnnnnnsssssnnnnnnnnnnnnnnnnnnnnnnn
nnnnnnnnnnnnnnnnnnnnssssssssssssssssssssnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn
All done!
```

#### MLFQ example 2

```
iiiiiiiiiiiiisssssssssssssnnnnnnnnnnnnnn
iiiiiiiiiiiiisssssssssssssnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnniiiiiiiiiiiiiiiiiiii
iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii
iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii
iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii
iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii
iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiisssssssssssssssssssssssssssssssssssssssssssssss
sssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssnnnnn
nnnnnnnnnissssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssss
ssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssnnnnnnnnnnnnnnnn
nnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnis
ssssssssssssnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnniissssssssssssssssssssss
sssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssnnn
nnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnssnnnnn
nnnnnnnnnnnnnnnnnnnnnsssssssnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn
nnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn
All done!
```

#### MLFQ example 3

```
iiiiiiiiiiiiisssssssssssssnnnnnnnnnnnnnn
iiiiiiiiiiiiisssssssssssssnnnnnnnnnnnnnniiiiiiiiiiiiisssssssssssssssssssssssssss
ssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssss
sssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssn
nnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnniiiiiiiiiiiiisssss
ssssssssnnnnnnnnnnnnnniiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii
iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii
iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii
iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii
iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiisssssssssssssssssssssssssss
ssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssss
ssssssssssssssssssssssnnnnnnnnnnnnnnsssssnnnnnnnnnnnnnnsssssssssssssssssssssssss
ssssssssssssssssssnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn
nnnnnnnnnnnnnnnnnnnnnsnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn
nnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn
nnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn
All done!
```
