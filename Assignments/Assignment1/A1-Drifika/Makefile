# COMP 3430 Operating Systems
# Winter 2025
# Franklin Bristow
#
# Students registered in this offering of the course are explicitly permitted
# to copy and use this Makefile for their own work.

CC = clang
CFLAGS = -Wall -Werror -Wextra -Wpedantic -g -D_FORTIFY_SOURCE=3

# Enable optional compilation flag
ifdef USE_LIBC_INSTEAD
    CFLAGS := -DUSE_LIBC_INSTEAD $(CFLAGS)
endif

# Define object files
OBJS = nqp_exfat.o

.PHONY: all clean

# Default target: Build programs based on USE_LIBC_INSTEAD flag
ifndef USE_LIBC_INSTEAD
all: cat ls paste main
else
all: cat main
endif

# Compile main.c (test program)
main: main.o $(OBJS)
	$(CC) $(CFLAGS) -o main main.o $(OBJS)

# Compile cat.c
cat: cat.o $(OBJS)
	$(CC) $(CFLAGS) -o cat cat.o $(OBJS)

# Compile ls.c (if applicable)
ls: ls.o $(OBJS)
	$(CC) $(CFLAGS) -o ls ls.o $(OBJS)

# Compile paste.c (if applicable)
paste: paste.o $(OBJS)
	$(CC) $(CFLAGS) -o paste paste.o $(OBJS)

# Compile object files
main.o: main.c nqp_exfat_types.h
	$(CC) $(CFLAGS) -c main.c

nqp_exfat.o: nqp_exfat.c nqp_exfat_types.h
	$(CC) $(CFLAGS) -c nqp_exfat.c

cat.o: cat.c nqp_io.h
	$(CC) $(CFLAGS) -c cat.c

ls.o: ls.c nqp_io.h
	$(CC) $(CFLAGS) -c ls.c

paste.o: paste.c nqp_io.h
	$(CC) $(CFLAGS) -c paste.c

# Clean up compiled files
clean:
	rm -rf main cat ls paste *.o
