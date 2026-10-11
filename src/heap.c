#include "heap.h"
#include <string.h>

void min_heap_init(MinHeap *heap, int capacity) {
    if (!heap) return;
    heap->size = 0;
    heap->capacity = (capacity > 0 && capacity <= MAX_HEAP_CAPACITY) ? capacity : MAX_HEAP_CAPACITY;
}

static void heap_swap(HeapElement *a, HeapElement *b) {
    HeapElement tmp = *a;
    *a = *b;
    *b = tmp;
}

static void heapify_up(MinHeap *heap, int idx) {
    while (idx > 0) {
        int parent = (idx - 1) / 2;
        if (heap->data[idx].priority < heap->data[parent].priority) {
            heap_swap(&heap->data[idx], &heap->data[parent]);
            idx = parent;
        } else {
            break;
        }
    }
}

static void heapify_down(MinHeap *heap, int idx) {
    int smallest = idx;
    int left = 2 * idx + 1;
    int right = 2 * idx + 2;

    if (left < heap->size && heap->data[left].priority < heap->data[smallest].priority) {
        smallest = left;
    }
    if (right < heap->size && heap->data[right].priority < heap->data[smallest].priority) {
        smallest = right;
    }

    if (smallest != idx) {
        heap_swap(&heap->data[idx], &heap->data[smallest]);
        heapify_down(heap, smallest);
    }
}

int min_heap_insert(MinHeap *heap, double priority, const Product *product) {
    if (!heap || !product || heap->size >= heap->capacity) return 0;

    int idx = heap->size++;
    heap->data[idx].priority = priority;
    heap->data[idx].product_id = product->id;
    heap->data[idx].product = *product;

    heapify_up(heap, idx);
    return 1;
}

int min_heap_extract_min(MinHeap *heap, HeapElement *out_elem) {
    if (!heap || heap->size <= 0) return 0;

    if (out_elem) {
        *out_elem = heap->data[0];
    }

    heap->data[0] = heap->data[heap->size - 1];
    heap->size--;

    if (heap->size > 0) {
        heapify_down(heap, 0);
    }
    return 1;
}

int min_heap_peek(const MinHeap *heap, HeapElement *out_elem) {
    if (!heap || heap->size <= 0) return 0;
    if (out_elem) {
        *out_elem = heap->data[0];
    }
    return 1;
}

int heap_find_top_k_by_rating(const Product *arr, int n, int k, Product *out_results) {
    if (!arr || n <= 0 || k <= 0 || !out_results) return 0;
    if (k > n) k = n;

    MinHeap heap;
    min_heap_init(&heap, k);

    for (int i = 0; i < n; i++) {
        double rating = arr[i].rating;
        if (heap.size < k) {
            min_heap_insert(&heap, rating, &arr[i]);
        } else if (rating > heap.data[0].priority) {
            min_heap_extract_min(&heap, NULL);
            min_heap_insert(&heap, rating, &arr[i]);
        }
    }

    int count = heap.size;
    /* Extract from heap (gives ascending order), put in out_results backwards for descending */
    for (int i = count - 1; i >= 0; i--) {
        HeapElement elem;
        min_heap_extract_min(&heap, &elem);
        out_results[i] = elem.product;
    }
    return count;
}

int heap_find_top_k_deals(const Product *arr, int n, int k, Product *out_results) {
    if (!arr || n <= 0 || k <= 0 || !out_results) return 0;
    if (k > n) k = n;

    MinHeap heap;
    min_heap_init(&heap, k);

    for (int i = 0; i < n; i++) {
        double discount_pct = 0.0;
        if (arr[i].mrp > 0) {
            discount_pct = ((double)(arr[i].mrp - arr[i].price) * 100.0) / (double)arr[i].mrp;
        }

        if (heap.size < k) {
            min_heap_insert(&heap, discount_pct, &arr[i]);
        } else if (discount_pct > heap.data[0].priority) {
            min_heap_extract_min(&heap, NULL);
            min_heap_insert(&heap, discount_pct, &arr[i]);
        }
    }

    int count = heap.size;
    for (int i = count - 1; i >= 0; i--) {
        HeapElement elem;
        min_heap_extract_min(&heap, &elem);
        out_results[i] = elem.product;
    }
    return count;
}
