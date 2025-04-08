#include <stdio.h>
2 #include <assert.h>
3 #include <pthread.h>
4 #include "common.h"
5 #include "common_threads.h"
6
7 void *mythread(void *arg) {
8 printf("%s\n", (char *) arg);
9 return NULL;
10 }
11
12 int
13 main(int argc, char *argv[]) {
14 pthread_t p1, p2;
15 int rc;
16 printf("main: begin\n");
17 Pthread_create(&p1, NULL, mythread, "A");
18 Pthread_create(&p2, NULL, mythread, "B");
19 // join waits for the threads to finish
20 Pthread_join(p1, NULL);
21 Pthread_join(p2, NULL);
22 printf("main: end\n");
23 return 0;
24 }
