CC = clang
CFLAGS = -Wall -Werror -Wextra -Wpedantic -O2 -D_FORTIFY_SOURCE=3 -g

.PHONY: clean

all: nqp_printer nqp_refiner nqp_sched_tasks nqp_list_insertion test_counter manprinter

nqp_printer: nqp_thread.o
	$(CC) $(CFLAGS) -o nqp_printer nqp_thread.o

nqp_refiner: nqp_thread.o
	$(CC) $(CFLAGS) -o nqp_refiner nqp_thread.o

nqp_sched_tasks: nqp_thread.o
	$(CC) $(CFLAGS) -o nqp_sched_tasks nqp_thread.o

nqp_list_insertion: nqp_list_insertion.o nqp_thread_locks.o
	$(CC) $(CFLAGS) -o nqp_list_insertion nqp_list_insertion.o nqp_thread_locks.o

test_counter: main.o nqp_thread_locks.o
	$(CC) $(CFLAGS) -o test_counter main.o nqp_thread_locks.o

manprinter: ManPrinter.o
	$(CC) $(CFLAGS) -o manprinter ManPrinter.o

main.o: main.c
	$(CC) $(CFLAGS) -c main.c

ManPrinter.o: ManPrinter.c
	$(CC) $(CFLAGS) -c ManPrinter.c

nqp_thread.o: nqp_thread.c
	$(CC) $(CFLAGS) -c nqp_thread.c

nqp_thread_locks.o: nqp_thread_locks.c
	$(CC) $(CFLAGS) -c nqp_thread_locks.c

nqp_list_insertion.o: nqp_list_insertion.c
	$(CC) $(CFLAGS) -c nqp_list_insertion.c

clean:
	rm -rf *.o nqp_printer nqp_refiner nqp_sched_tasks nqp_list_insertion test_counter manprinter
