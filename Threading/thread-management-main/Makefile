# COMP 3430 Operating Systems
# Winter 2025
# Franklin Bristow
#
# Students registered in this offering of the course are explicitly permitted
# to copy and use this Makefile for their own work.

CC = clang
CFLAGS = -Wall -Werror -Wextra -Wpedantic -g -D_FORTIFY_SOURCE=3
LDLIBS = -lpthread

.PHONY: clean

all: threads deadlock condition-variables \
	 critical-sections/textbook-sample \
	 critical-sections/textbook-sample-modified-a \
	 critical-sections/textbook-sample-modified-b\
	 critical-sections/list-traversal-a \
	 critical-sections/list-traversal-b \
	 critical-sections/list-insertion \
	 thr_pool/thread_madness \
	 3430-pool/thread-madness 3430-pool/test-queue

thr_pool/thread_madness: thr_pool/thr_pool.o

3430-pool/thread-madness: 3430-pool/3430-pool.o 3430-pool/job-queue.o

3430-pool/test-queue: 3430-pool/job-queue.o

clean:
	rm -rf threads deadlock condition-variables \
	 critical-sections/textbook-sample \
	 critical-sections/textbook-sample-modified-a \
	 critical-sections/textbook-sample-modified-b\
	 critical-sections/list-traversal-a \
	 critical-sections/list-traversal-b \
	 critical-sections/list-insertion \
	 thr_pool/thread_madness \
	 thr_pool/thr_pool.o \
	 3430-pool/thread-madness 3430-pool/test-queue \
	 3430-pool/3430-pool.o 3430-pool/job-queue.o
