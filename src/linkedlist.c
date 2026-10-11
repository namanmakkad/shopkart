#include "linkedlist.h"
#include <stdlib.h>

void linkedlist_init(LinkedList *list) {
    if (!list) return;
    list->head = NULL;
    list->tail = NULL;
    list->size = 0;
}

int linkedlist_append(LinkedList *list, int product_id) {
    if (!list || product_id <= 0) return 0;

    /* Prevent duplicates in wishlist */
    if (linkedlist_contains(list, product_id)) {
        return 1;
    }

    WishlistNode *node = (WishlistNode *)malloc(sizeof(WishlistNode));
    if (!node) return 0;

    node->product_id = product_id;
    node->next = NULL;

    if (!list->head) {
        list->head = node;
        list->tail = node;
    } else {
        list->tail->next = node;
        list->tail = node;
    }
    list->size++;
    return 1;
}

int linkedlist_remove(LinkedList *list, int product_id) {
    if (!list || !list->head) return 0;

    WishlistNode *curr = list->head;
    WishlistNode *prev = NULL;

    while (curr) {
        if (curr->product_id == product_id) {
            if (prev) {
                prev->next = curr->next;
            } else {
                list->head = curr->next;
            }

            if (curr == list->tail) {
                list->tail = prev;
            }

            free(curr);
            list->size--;
            return 1;
        }
        prev = curr;
        curr = curr->next;
    }
    return 0;
}

int linkedlist_contains(const LinkedList *list, int product_id) {
    if (!list) return 0;
    WishlistNode *curr = list->head;
    while (curr) {
        if (curr->product_id == product_id) {
            return 1;
        }
        curr = curr->next;
    }
    return 0;
}

int linkedlist_to_array(const LinkedList *list, int *out_ids, int max_capacity) {
    if (!list || !out_ids || max_capacity <= 0) return 0;

    int count = 0;
    WishlistNode *curr = list->head;
    while (curr && count < max_capacity) {
        out_ids[count++] = curr->product_id;
        curr = curr->next;
    }
    return count;
}

int linkedlist_size(const LinkedList *list) {
    return list ? list->size : 0;
}

void linkedlist_free(LinkedList *list) {
    if (!list) return;
    WishlistNode *curr = list->head;
    while (curr) {
        WishlistNode *next = curr->next;
        free(curr);
        curr = next;
    }
    list->head = NULL;
    list->tail = NULL;
    list->size = 0;
}
