---
title: COMP 3430 Operating Systems
subtitle: Printing program
author: Franklin Bristow
date: Winter 2025
toccolor: blue
toc: true
header-includes:
- \usepackage{awesomebox}
pandoc-latex-environment:
    noteblock: [note]
---

Overview
========

This is a short program that only has the job of printing out a single
character, forever. This program is effectively the same as the program that's
called `yes` (see `man yes`), but it's easier for us to see the insides of the
program if we have the source code.

This program also demonstrates using `getopt`, though you can find more examples
of using `getopt` in the manual page (`man 3 getopt`).

::: note 

You can find the source code for the real `yes` program [on GitHub].

[on GitHub]: https://github.com/coreutils/coreutils/blob/master/src/yes.c

:::

Building and running
====================

You can build this program using `make`:

```bash
make
```

You can run this program on the command line. The program takes on option (`-c`)
telling it which character to print out (forever). For example, this will print
out the letter 'A' forever:

```bash
./print -c A
```

Pausing and ending the program
------------------------------

We'll talk about this in class, but for completeness:

* You must terminate this process by pressing Ctrl+C on your keyboard.
* You can pause this process by pressing Ctrl+Z on your keyboard.
* You can "put this process in the background" (restart the process, but you can
  still type commands into the shell) by entering the command `bg`.
* You can "bring this process to the foreground" (give control back to the
  process) by entering the command `fg` *only if* the process was running in the
  background.

