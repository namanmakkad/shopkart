#include "stack.h"
#include <string.h>

void stack_init(Stack *s) {
    if (!s) return;
    s->top = -1;
}

int stack_push(Stack *s, const Product *product, int quantity) {
    if (!s || !product) return 0;
    if (s->top >= MAX_STACK_CAPACITY - 1) {
        return 0; /* Stack full */
    }

    s->top++;
    s->items[s->top].product = *product;
    s->items[s->top].quantity = quantity;
    return 1;
}

int stack_pop(Stack *s, CartUndoItem *out_item) {
    if (!s || s->top < 0) return 0;

    if (out_item) {
        *out_item = s->items[s->top];
    }
    s->top--;
    return 1;
}

int stack_peek(const Stack *s, CartUndoItem *out_item) {
    if (!s || s->top < 0) return 0;

    if (out_item) {
        *out_item = s->items[s->top];
    }
    return 1;
}

int stack_is_empty(const Stack *s) {
    return (!s || s->top < 0);
}

int stack_size(const Stack *s) {
    return s ? (s->top + 1) : 0;
}

void stack_clear(Stack *s) {
    if (!s) return;
    s->top = -1;
}
