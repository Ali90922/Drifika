---
title: COMP 3430 Operating Systems
subtitle: IPC
author: Franklin Bristow
date: Winter 2025
toccolor: blue
toc: true
---

Overview
========

This is an example that demonstrates registering signal handlers, sending
signals to processes (you can do this with the `kill` command), and sending
signals to processes from code (with the `kill(2)` system call).

This is also an example of using pipes! Specifically, this example will work us
through calling the `pipe(2)` system call (which is modifying our open file
table!) and the calling `fork(2)`. After we've duplicated our PCB, we'll send
messages between the parent and child process.

This is also an example of what can go wrong when you're using pipes!
Specifically, this will have two processes (a parent and child)
sending/receiving a message, but neither of them will be able to exit.

Building and running
====================

This repository has two files: `signal.c` and `pipes.c`.

You can build these programs with `make`:

```bash
make
```

`signal.c`
----------

You can run this program on the command line:

```bash
./signal
```

The simplest way to send a signal to this program is to press <kbd>Ctrl+C</kbd>
on your keyboard. This sends an "interrupt" signal to the process.

This program will now eventually end by itself:

1. We are registering a signal handler in the parent process for `SIGINT`. The
   signal handler has the responsibility of setting a flag when the signal is
   received to indicate to the process that it should stop spinning around.
2. We `fork()`, the child process does some "hard work" (it goes to sleep), then
   sends the signal to the parent. The parent handles the signal and sets the
   flag, allowing it to leave the loop it was blocked on before.
3. The child process proceeds to do some more "hard work" (it goes back to
   sleep) and the parent blocks `wait()`ing for the child to `exit()`.
   Eventually the child process wakes up and `exit()`s, allowing the parent
   process to get unblocked from `wait()` and it also `exit()`s.

You can send signals to this running program **from the same machine** (if you
are fast enough! If you're connecting to Aviary, you need to connect directly to
the same bird machine) using the `kill` command:

```bash
# Really terminate this program on Aviary:
kill -9 $PID # or
kill -KILL $PID
# Really terminate this program on (at least) my (Franklin's) computer:
kill -SIGKILL $PID 
```

Check out which signals you can send to a process and the default action or
behaviour by looking at section 7 of the manual pages for `signal`:

```bash
man 7 signal
```

`pipes.c`
---------

You can run this program on the command line by passing an argument to the
program. The argument to the program should be the name of the program that it
will start with `exec`:

```bash
./pipes cat
```

This program will print out a message indicating that it's going to start a
child process, then halt. When the process is blocked, the parent process is
waiting for input on standard input. You can type stuff in and press
<kbd>Enter</kbd>. The parent process will `write()` anything that it `read()` on
standard input to the write end of the pipe. The child process (which is
currently `cat` that we `exec`-ed!) will receive this input on its standard
input by `read()`ing standard input, then `write()` it to standard output.

You can end this program by pressing <kbd>Ctrl+D</kbd> on your keyboard.
Pressing <kbd>Ctrl+D</kbd> will "flush" standard input (it unblocks whatever was
blocked on `read(STDIN_FILENO`!). The `read` call will return 0 (it `read()`
nothing from standard input), allowing loops that block on `read()`ing standard
input to exit.

`frozen-pipes.c`
----------------

You can run this program on the command line, it takes no arguments:

```bash
./frozen-pipes
```

In its current state this program will get blocked and then never be able to
proceed to finishing. We're going to explore *why* this program is getting
blocked, then try fixing the problem in a variety of different ways in class.

For now you should end this program by pressing <kbd>Ctrl+C</kbd> on your
keyboard.

One of the tools we're going to use to investigate this program is called
[`strace`]. `strace` is a "linux syscall tracer". We've used this before in
class when beginning to look at `read` and `write`. This time we're going to use
it to help us see what, exactly, our parent and child process are getting
blocked on.

`strace` is a Linux-only tool. If you want to follow along in class with this
you're going to need to do this on a Linux system (e.g., Aviary, WSL,
macOS+lima).

Running a program with `strace` looks a lot like running it on the command line
in general:

```bash
strace ./frozen-pipes
```

We're going to be using a few different options on `strace` to help us figure
out what's happening:

* `-e trace=%desc,%file,%process`: this will limit the output from `strace` to
    1. System calls dealing with file descriptors (`%desc`).
    2. System calls taking file names as arguments (`%file`).
    3. System calls dealing with processes (`%process`).
* `--follow-forks`: this will cause `strace` to not just print system call
  information for the current process, but also any processes that the current
  process `fork`s.

`strace` is a really powerful tool and has a lot of options. You should see the
manual pages (`man strace`) or the [`strace` home page][`strace`] for more
information about how you can run `strace`.

[`strace`]: https://strace.io/
