---
title: COMP 3430 Operating Systems
subtitle: "Assignment 3: Concurrency!"
date: Winter 2025
---

Overview
=========

This directory contains the following:

* This `README.md` file (you're reading it!).
* A `Makefile` that can build the sample code.
* Several interfaces that describe how concurrency could be used
  (`nqp_thread.h`, `nqp_thread_locks.h`, and `nqp_thread_sched.h`).
* An initial implementation of some code that uses (parts of) the concurrency
  defined by these interfaces (`nqp_printer.c`).

Building and running
====================

The only runnable program in this directory is `nqp_printer.c`.

You can compile this program on the command line:

```bash
make
```

You can run this program on the command line by passing the volume that you
would like to have the shell use as a root directory:

```bash
./nqp_printer
```

This code, as is, will crash: the default return value from `nqp_thread_create`
is `NULL`, and the code in `nqp_thread_join` has an `assert`ion that the passed
argument is not `NULL`. You must minimally implement `nqp_thread_create` for
this code to (as is) not crash.
