#pragma once

// forward declaration of our list type; we want to hide implementation details
// from other code using this list library.
typedef struct INT_LIST int_list;

/**
 * Create a new, empty list.
 *
 * Returns: a new, empty list. May return NULL on error.
 */
int_list *create_list(void);

/**
 * Destroy a list. List should not be used after being passed to this function.
 *
 * Parameters:
 *  * list: the list to destroy. Must not be NULL. Must be a valid list.
 * Returns: NULL on success, or the pointer to the list on failure.
 */
int_list *destroy_list(int_list *list);

/**
 * Inserts an item at the beginning of the list.
 *
 * Parameters:
 *  * list: the list to insert to. Must not be NULL. Must be a valid list.
 *  * item: the item to insert.
 * Returns: the new size of the list, or -1 to indicate error.
 */
int insert_front(int_list *list, int item);

/**
 * How many elements are in the list?
 *
 * Parameters:
 *  * list: the list to count elements for. Must not be NULL. Must be a valid
 *          list. this function will not modify the list.
 * Returns: the current size of the list (0 or 1), or -1 to indicate error.
 */
int list_size(const int_list *list);
