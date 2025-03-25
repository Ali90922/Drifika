# COMP 3430 Operating Systems
# Winter 2025
# Franklin Bristow
#
# Students registered in this offering of the course are explicitly permitted
# to copy and use this Makefile for their own work.

CC = clang
CFLAGS = -Wall -Werror -Wextra -Wpedantic -O2 -D_FORTIFY_SOURCE=3 -g

.PHONY: clean

all: nqp_printer nqp_refiner nqp_thread_locks.o nqp_sched_tasks

nqp_printer: nqp_thread.o

nqp_refiner: nqp_thread.o

nqp_sched_tasks: nqp_thread.o

clean:
	rm -rf nqp_thread_locks.o nqp_thread.o nqp_printer nqp_refiner \
		nqp_sched_tasks
