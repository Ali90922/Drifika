# COMP 3430 Operating Systems
# Winter 2025
# Franklin Bristow
#
# Students registered in this offering of the course are explicitly permitted
# to copy and use this Makefile for their own work.

CC = clang
CFLAGS = -Wall -Werror -Wextra -Wpedantic -O2 -D_FORTIFY_SOURCE=3 -g
LDFLAGS = -pthread

.PHONY: all clean

# Add the new target "test_counter" to the list of all targets.
all: nqp_printer nqp_refiner nqp_thread_locks.o nqp_sched_tasks test_counter

# Existing targets (assuming these link against nqp_thread.o)
nqp_printer: nqp_thread.o
	$(CC) $(CFLAGS) -o nqp_printer nqp_thread.o

nqp_refiner: nqp_thread.o
	$(CC) $(CFLAGS) -o nqp_refiner nqp_thread.o

nqp_sched_tasks: nqp_thread.o
	$(CC) $(CFLAGS) -o nqp_sched_tasks nqp_thread.o

# New target for compiling main.c with your custom lock implementation.
test_counter: main.o nqp_thread_locks.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o test_counter main.o nqp_thread_locks.o

# Rules to build object files
main.o: main.c
	$(CC) $(CFLAGS) -c main.c

nqp_thread_locks.o: nqp_thread_locks.c nqp_thread_locks.h
	$(CC) $(CFLAGS) -c nqp_thread_locks.c

nqp_thread.o: nqp_thread.c nqp_thread.h
	$(CC) $(CFLAGS) -c nqp_thread.c

clean:
	rm -rf nqp_thread_locks.o nqp_thread.o main.o nqp_printer nqp_refiner \
		nqp_sched_tasks test_counter
