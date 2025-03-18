COMP 3430 Thread pool example
=============================

This contains an initial set of functions and some client code for a thread pool
implementation. The code here is inspired by the [Oracle Thread Pool
Implementation], though little to no code from that example still exists here.

[Oracle Thread Pool Implementation]:
https://docs.oracle.com/cd/E19253-01/816-5137/ggedn/index.html

Building
--------

This example uses `make`, you can run

```bash
make -C ..
```

in the current directory to build the example (this uses the `Makefile` in the
parent directory).

Running
-------

[Building] produces several executable files: `test-queue` and `thread-madness`.
You can run the example by running:

```bash
./test-queue      # should print nothing
./thread-madness  # will print something
```

in the current directory. 

Debugging
---------

We will spend some time using our debugger to work with multi-threaded code. We
will use `lldb` in class:

```bash
lldb ./thread-madness
```

We'll do some work related to [examining thread state] like listing the threads
in the current process and switching between them to examine their current
state using the commands:

* `thread list`: lists the threads in the current process.
* `thread select N`: switches to thread #N
* `up` and `down`: navigate through the call stack for the currently selected
  thread.
* `bt`: print out the call stack for the currently selected thread.
* `p VARIABLE`: print out the value of `VARIABLE` in the current stack from for
  the currently selected thread.

[examining thread state]: https://lldb.llvm.org/use/tutorial.html#examining-thread-state

WARNING
-------

The same warning that was in the [Oracle thread pool example](../thr_pool)
applies to code in this folder: This code is for demonstration purposes only,
you really shouldn't use it for anything meaningful. In fact, the warning
applies *even more* to this code, explicitly because I (me, Franklin) haven't
handled nearly as much [esoteric] pthread-related business as what was in the
original code by Oracle.

[esoteric]: https://en.wiktionary.org/wiki/esoteric
