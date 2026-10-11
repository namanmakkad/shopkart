#ifndef QUEUE_H
#define QUEUE_H

#include "product.h"

#define MAX_RECENT_VIEWED 10

/*
 * Circular Queue for Recently Viewed Products (Cap: 10, Evict Oldest).
 * Time Complexity: Enqueue O(1) (or O(k) when deduplicating), Dequeue O(1).
 * Space Complexity: O(k) fixed circular buffer (k = 10).
 */
typedef struct {
    int product_ids[MAX_RECENT_VIEWED];
    int head;  /* read pointer */
    int tail;  /* write pointer */
    int count; /* current number of elements */
} CircularQueue;

/* Initialize circular queue */
void queue_init(CircularQueue *q);

/*
 * Add a product ID to recently viewed.
 * If product already in queue, moves it to front.
 * If queue is full (count == 10), evicts the oldest element (FIFO).
 */
void queue_enqueue(CircularQueue *q, int product_id);

/* Dequeue the oldest product ID */
int queue_dequeue(CircularQueue *q, int *out_product_id);

/* Check if product is currently present in the queue */
int queue_contains(const CircularQueue *q, int product_id);

/* Get all items in chronological order: index 0 is newest, count-1 is oldest */
int queue_get_items_newest_first(const CircularQueue *q, int out_product_ids[MAX_RECENT_VIEWED]);

/* Check if queue is empty */
int queue_is_empty(const CircularQueue *q);

/* Get number of items */
int queue_size(const CircularQueue *q);

#endif /* QUEUE_H */
