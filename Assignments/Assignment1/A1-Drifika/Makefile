# COMP 3430 Operating Systems
# Winter 2025
# Franklin Bristow

CC = clang
CFLAGS = -Wall -Werror -Wextra -Wpedantic -g -D_FORTIFY_SOURCE=3

# Enable optional compilation flag
ifdef USE_LIBC_INSTEAD
	CFLAGS := -DUSE_LIBC_INSTEAD $(CFLAGS)
endif

# Define object files
OBJS = nqp_exfat.o

# Default target: Build everything
.PHONY: all clean

all: main cat

# Compile main.c (test program)
main: main.o $(OBJS)
	$(CC) $(CFLAGS) -o main main.o $(OBJS)

# Compile cat.c (existing program)
cat: cat.o $(OBJS)
	$(CC) $(CFLAGS) -o cat cat.o $(OBJS)

# Compile object files
main.o: main.c nqp_exfat_types.h
	$(CC) $(CFLAGS) -c main.c

nqp_exfat.o: nqp_exfat.c nqp_exfat_types.h
	$(CC) $(CFLAGS) -c nqp_exfat.c

cat.o: cat.c
	$(CC) $(CFLAGS) -c cat.c

# Clean up compiled files
clean:
	rm -rf main cat *.o
