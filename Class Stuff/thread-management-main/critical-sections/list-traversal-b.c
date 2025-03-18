#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

typedef struct NODE {
    char *value;
    struct NODE *next;
} node;

node *head;
node *curr;

void build_list(void);

void *thread_traverse(void *args)
{
    (void) args;
    curr = head;
    while ( curr != NULL )
    {
        printf("%s\n", curr->value);
        curr = curr->next;
    }
    return NULL;
}

int main(void)
{
    pthread_t t1, t2;

    build_list();

    pthread_create(&t1, NULL, thread_traverse, NULL );
    pthread_create(&t2, NULL, thread_traverse, NULL );

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    return EXIT_SUCCESS;
}

void build_list(void)
{
    char number[4];
    node *temp;
    head = malloc(sizeof(node));
    head->value = strdup("Hello");
    head->next = NULL;

    temp = head;
    head = malloc(sizeof(node));
    head->value = strdup("world!");
    head->next = temp;

    for (int i = 0; i < 100; i++)
    {
        temp = head;
        head = malloc(sizeof(node));
        snprintf(number, 4, "%03d", i);
        head->value = strdup(number);
        head->next = temp;
    }
}
