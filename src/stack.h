#ifndef STACK_H
#define STACK_H

#include "product.h"

#define MAX_STACK_CAPACITY 64

typedef struct {
    Product product;
    int quantity;
} CartUndoItem;

/*
 * LIFO Stack for Cart "Undo Remove" operations.
 * Time Complexity: Push O(1), Pop O(1), Peek O(1).
 * Space Complexity: O(k) where k is number of items stored.
 */
typedef struct {
    CartUndoItem items[MAX_STACK_CAPACITY];
    int top; /* -1 when empty */
} Stack;

/* Initialize stack to empty state */
void stack_init(Stack *s);

/* Push an item onto the top of the stack. Returns 1 on success, 0 if full. */
int stack_push(Stack *s, const Product *product, int quantity);

/* Pop the most recently removed item. Returns 1 on success, 0 if empty. */
int stack_pop(Stack *s, CartUndoItem *out_item);

/* Peek at the top item without removing. Returns 1 if present, 0 if empty. */
int stack_peek(const Stack *s, CartUndoItem *out_item);

/* Check if stack is empty */
int stack_is_empty(const Stack *s);

/* Get number of elements on the stack */
int stack_size(const Stack *s);

/* Clear all elements from stack */
void stack_clear(Stack *s);

#endif /* STACK_H */
