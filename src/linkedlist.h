#ifndef LINKEDLIST_H
#define LINKEDLIST_H

#include "product.h"

/*
 * Singly Linked List Node for Wishlist.
 */
typedef struct WishlistNode {
    int product_id;
    struct WishlistNode *next;
} WishlistNode;

/*
 * Singly Linked List for User Wishlist.
 * Time Complexity: Append O(1) with tail pointer, Remove O(n), Lookup O(n).
 * Space Complexity: O(n) dynamic nodes.
 */
typedef struct {
    WishlistNode *head;
    WishlistNode *tail;
    int size;
} LinkedList;

/* Initialize empty linked list */
void linkedlist_init(LinkedList *list);

/* Append product ID to the end of the list. Returns 1 on success, 0 on memory failure. */
int linkedlist_append(LinkedList *list, int product_id);

/* Remove first occurrence of product ID from list. Returns 1 if removed, 0 if not found. */
int linkedlist_remove(LinkedList *list, int product_id);

/* Check if list contains product ID */
int linkedlist_contains(const LinkedList *list, int product_id);

/* Dump all product IDs into an array. Returns number of items written. */
int linkedlist_to_array(const LinkedList *list, int *out_ids, int max_capacity);

/* Get number of elements */
int linkedlist_size(const LinkedList *list);

/* Free all nodes in the list */
void linkedlist_free(LinkedList *list);

#endif /* LINKEDLIST_H */
