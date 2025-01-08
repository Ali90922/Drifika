#include <assert.h>
#include <stdlib.h>
#include "list.h"

typedef struct INT_NODE {
    struct INT_NODE *next;
    int value;
} int_node;

struct INT_LIST {
    int_node *head;
    int count;
};


