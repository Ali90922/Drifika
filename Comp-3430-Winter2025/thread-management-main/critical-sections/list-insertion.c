#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

void build_list(void);
void print_list(void);

typedef struct NODE {
    char *value;
    struct NODE *next;
    // lock the node we're going to modify
    // this is the best option in terms of
    // maximizing concurrency.
    // if I have 100000 nodes in my list, I have
    // 100000 locks
    pthread_mutex_t node_lock;
} node;

typedef struct LIST
{
    node *head;
    // put the lock on the head of the list.
    // if I have 100000 nodes in my list, I have
    // just one lock.
    pthread_mutex_t insert_lock;
} list;

typedef struct NODE_INSERT
{
    node *insert_after;
    node *to_insert;
} node_insert;

node *head;
// file-scope option for lock:
pthread_mutex_t insert_lock;
// we're now limited to exactly one thread of execution
// entering the critical section at a time, regardless
// of how many lists or nodes that we're trying to modify.

void *thread_insert_after(void *args)
{
    node_insert *insert_operation = (node_insert *) args;

    // function-scope lock doesn't actually lock anything
    //pthread_mutex_t insert_lock = PTHREAD_MUTEX_INITIALIZER; // function-scope lock

    pthread_mutex_lock( &insert_lock ); // file-scope lock

    // pthread_mutex_lock( &insert_operation->insert_after->node_lock ); // node-scope lock

    insert_operation->to_insert->next = insert_operation->insert_after->next;
    insert_operation->insert_after->next = insert_operation->to_insert;
    
    // pthread_mutex_lock( &insert_operation->insert_after->node_lock ); // node-scope lock

    pthread_mutex_unlock( &insert_lock ); // file-scope lock

    return NULL;
}

int main(void)
{
#define THREADS 100000
    pthread_t threads[THREADS];
    node_insert inserts[THREADS] = {0};
    char number[4];

    build_list();

    for (int i = 0; i < THREADS; i++)
    {
        inserts[i].insert_after = head;
        inserts[i].to_insert = malloc(sizeof(node));
        snprintf(number, 4, "%03d", i);
        inserts[i].to_insert->value = strdup(number);
    }
    for (int i = 0; i < THREADS; i++)
    {
        pthread_create(&threads[i], NULL, thread_insert_after, &inserts[i] );
    }

    for (int i = 0; i < THREADS; i++)
    {
        pthread_join(threads[i], NULL);
    }

    print_list();

    return EXIT_SUCCESS;
}

void build_list(void)
{
    head = malloc(sizeof(node));
    head->value = strdup("Hello");
    head->next = NULL;
}

void print_list(void)
{
    node *curr = head;
    while (curr != NULL)
    {
        printf("%s\n", curr->value);
        curr = curr->next;
    }
}
