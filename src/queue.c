#include "queue.h"
#include <string.h>

void queue_init(CircularQueue *q) {
    if (!q) return;
    q->head = 0;
    q->tail = 0;
    q->count = 0;
    for (int i = 0; i < MAX_RECENT_VIEWED; i++) {
        q->product_ids[i] = 0;
    }
}

int queue_contains(const CircularQueue *q, int product_id) {
    if (!q || q->count == 0) return 0;
    for (int i = 0; i < q->count; i++) {
        int idx = (q->head + i) % MAX_RECENT_VIEWED;
        if (q->product_ids[idx] == product_id) {
            return 1;
        }
    }
    return 0;
}

void queue_enqueue(CircularQueue *q, int product_id) {
    if (!q || product_id <= 0) return;

    /* If item already present, remove it so it can be moved to newest */
    if (queue_contains(q, product_id)) {
        int temp[MAX_RECENT_VIEWED];
        int new_count = 0;
        for (int i = 0; i < q->count; i++) {
            int idx = (q->head + i) % MAX_RECENT_VIEWED;
            if (q->product_ids[idx] != product_id) {
                temp[new_count++] = q->product_ids[idx];
            }
        }
        /* Repopulate queue */
        q->head = 0;
        q->tail = 0;
        q->count = 0;
        for (int i = 0; i < new_count; i++) {
            q->product_ids[q->tail] = temp[i];
            q->tail = (q->tail + 1) % MAX_RECENT_VIEWED;
            q->count++;
        }
    }

    /* If queue is full, evict oldest element at head (FIFO eviction) */
    if (q->count == MAX_RECENT_VIEWED) {
        q->head = (q->head + 1) % MAX_RECENT_VIEWED;
        q->count--;
    }

    /* Append to tail */
    q->product_ids[q->tail] = product_id;
    q->tail = (q->tail + 1) % MAX_RECENT_VIEWED;
    q->count++;
}

int queue_dequeue(CircularQueue *q, int *out_product_id) {
    if (!q || q->count == 0) return 0;

    if (out_product_id) {
        *out_product_id = q->product_ids[q->head];
    }
    q->head = (q->head + 1) % MAX_RECENT_VIEWED;
    q->count--;
    return 1;
}

int queue_get_items_newest_first(const CircularQueue *q, int out_product_ids[MAX_RECENT_VIEWED]) {
    if (!q || q->count == 0 || !out_product_ids) return 0;

    int res_count = 0;
    /* Iterate from newest (tail - 1) backwards to oldest (head) */
    for (int i = q->count - 1; i >= 0; i--) {
        int idx = (q->head + i) % MAX_RECENT_VIEWED;
        out_product_ids[res_count++] = q->product_ids[idx];
    }
    return res_count;
}

int queue_is_empty(const CircularQueue *q) {
    return (!q || q->count == 0);
}

int queue_size(const CircularQueue *q) {
    return q ? q->count : 0;
}
