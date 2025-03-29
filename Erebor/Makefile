# COMP 3430 Operating Systems
# Winter 2025
# Franklin Bristow
#
# Students registered in this offering of the course are explicitly permitted
# to copy and use this Makefile for their own work.

CC = clang
CFLAGS = -Wall -Werror -Wextra -Wpedantic -O2 -D_FORTIFY_SOURCE=3 -g

.PHONY: clean

all: nqp_printer nqp_refiner nqp_sched_tasks nqp_list_insertion test_counter manprinter

nqp_printer: nqp_thread.o

nqp_refiner: nqp_thread.o

nqp_sched_tasks: nqp_thread.o

nqp_list_insertion: nqp_thread.o nqp_thread_locks.o

manprinter: ManPrinter.o
	$(CC) $(CFLAGS) -o manprinter ManPrinter.o



# New target for compiling main.c with the custom lock implementation.
test_counter: main.o nqp_thread_locks.o
	$(CC) $(CFLAGS) -o test_counter main.o nqp_thread_locks.o

# Rule to build main.o from main.c
main.o: main.c
	$(CC) $(CFLAGS) -c main.c

# Rule to build ManPrinter.o from ManPrinter.c
ManPrinter.o: ManPrinter.c
	$(CC) $(CFLAGS) -c ManPrinter.c


clean:
	rm -rf nqp_thread_locks.o nqp_thread.o main.o ManPrinter.o \
		nqp_printer nqp_refiner nqp_sched_tasks nqp_list_insertion test_counter manprinter

# Dont mess around with the man printer