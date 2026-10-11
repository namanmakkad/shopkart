#ifndef HEAP_H
#define HEAP_H

#include "product.h"

#define MAX_HEAP_CAPACITY 64

typedef struct {
    double priority; /* rating or discount percentage */
    int product_id;
    Product product;
} HeapElement;

/*
 * Binary Min-Heap data structure.
 * Time Complexity: Insert O(log k), Extract-Min O(log k), Peek O(1).
 * Space Complexity: O(k) memory.
 */
typedef struct {
    HeapElement data[MAX_HEAP_CAPACITY];
    int size;
    int capacity;
} MinHeap;

/* Initialize min-heap with designated capacity (max MAX_HEAP_CAPACITY) */
void min_heap_init(MinHeap *heap, int capacity);

/* Insert element into min-heap. Returns 1 on success, 0 if full. */
int min_heap_insert(MinHeap *heap, double priority, const Product *product);

/* Extract minimum element from heap. Returns 1 on success, 0 if empty. */
int min_heap_extract_min(MinHeap *heap, HeapElement *out_elem);

/* Peek at minimum element without removal */
int min_heap_peek(const MinHeap *heap, HeapElement *out_elem);

/*
 * Top-K Highest Rated Products without sorting the full array:
 * Maintains a min-heap of size K. For each element in arr, if rating > heap.min,
 * replaces min. At end, heap contains the K highest elements.
 * Time Complexity: O(n log k) vs O(n log n) full sort.
 * Space Complexity: O(k) auxiliary heap.
 * Returns count of elements written to out_results.
 */
int heap_find_top_k_by_rating(const Product *arr, int n, int k, Product *out_results);

/*
 * Top-K Highest Deals (by discount percentage) in O(n log k):
 * Time Complexity: O(n log k).
 * Space Complexity: O(k).
 */
int heap_find_top_k_deals(const Product *arr, int n, int k, Product *out_results);

#endif /* HEAP_H */
