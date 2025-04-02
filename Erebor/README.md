# COMP 3430 Assignment 3: Concurrency, Threads & Scheduling

## Overview
This assignment implements a user-level threading library using `ucontext_t`. It includes:
- **Thread Management:** Creating, yielding, and exiting threads.
- **Scheduling Policies:** Two-thread (default), FIFO, Round-Robin (RR), and a basic Multi-Level Feedback Queue (MLFQ).
- **Locks (optional):** Spin locks using atomic operations.

## File Structure
- `nqp_thread.h/c` – Thread interface and implementation.
-- `nqp_thread_lock.c/h` -  
- `nqp_thread_sched.h/c` – Scheduling policy implementations.
- `main.c` – Sample program demonstrating thread usage.
- `Makefile` – Build instructions.

## Compilation & Running
```bash
make
./nqp_threads
./nqp_refiner
```




###
