#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "nqp_thread.h"
#include "nqp_thread_sched.h"
#include "nqp_thread_locks.h"

void build_list(void);
void print_list(int);

typedef struct NODE {
    char *value;
    struct NODE *next;
    nqp_mutex_t *node_lock;
} node;

typedef struct NODE_INSERT
{
    node *insert_after;
    node *to_insert;
} node_insert;

node *head;

void thread_insert_after(void *args)
{
    node_insert *insert_operation = (node_insert *) args;

    nqp_thread_mutex_lock( insert_operation->insert_after->node_lock ); 

    insert_operation->to_insert->next = insert_operation->insert_after->next;
    nqp_yield( ); // oops; this is the worst-case scenario, every thread is
                  // always "interrupted" in between getting the next node,
                  // so all nodes that we are going to insert are lost except
                  // the last thread to finish running.
    insert_operation->insert_after->next = insert_operation->to_insert;
    
    nqp_thread_mutex_unlock( insert_operation->insert_after->node_lock ); 

    nqp_exit( );
}

int main(void)
{
#define THREADS 20
    nqp_thread_t *threads[THREADS] = {0};
    node_insert inserts[THREADS] = {0};
    char number[5] = {0};

    build_list();

    for (int i = 0; i < THREADS; i++)
    {
        inserts[i].insert_after = head;
        inserts[i].to_insert = malloc(sizeof(node));
        snprintf(number, 4, "%03d", i);
        inserts[i].to_insert->value = strdup(number);
        inserts[i].to_insert->node_lock = nqp_thread_mutex_init();
    }
    for (int i = 0; i < THREADS; i++)
    {
        threads[i] = nqp_thread_create(thread_insert_after, &inserts[i] );
    }

    nqp_sched_init( NQP_SP_RR, NULL );
    nqp_sched_start( );

    for (int i = 0; i < THREADS; i++)
    {
        nqp_thread_join(threads[i]);
    }

    print_list( THREADS + 1 );

    return EXIT_SUCCESS;
}

void build_list(void)
{
    head = malloc(sizeof(node));
    head->value = strdup("head");
    head->next = NULL;
    head->node_lock = nqp_thread_mutex_init();
}

void print_list(int expected_count)
{
    node *curr = head;
    int count = 0;
    while (curr != NULL)
    {
        printf("%s\n", curr->value);
        curr = curr->next;
        count++;
    }

    printf("Expected size: [%d], actual size: [%d]\n", expected_count, count);
}
