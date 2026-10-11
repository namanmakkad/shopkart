#include "sort.h"
#include <stdlib.h>
#include <string.h>

/*
 * Compare two products based on the given sort key.
 * Time Complexity: O(1)
 * Space Complexity: O(1)
 */
int compare_products(const Product *a, const Product *b, SortKey key, unsigned long long *comparisons) {
    if (comparisons) {
        (*comparisons)++;
    }

    if (!a || !b) return 0;

    switch (key) {
        case SORT_KEY_PRICE_ASC:
            if (a->price != b->price) {
                return a->price - b->price;
            }
            return a->id - b->id;

        case SORT_KEY_PRICE_DESC:
            if (a->price != b->price) {
                return b->price - a->price;
            }
            return a->id - b->id;

        case SORT_KEY_RATING_DESC:
            if (a->rating > b->rating) return -1;
            if (a->rating < b->rating) return 1;
            /* Multi-key tie breaker: price ascending */
            if (a->price != b->price) {
                return a->price - b->price;
            }
            return a->id - b->id;

        case SORT_KEY_DISCOUNT_DESC: {
            double disc_a = (a->mrp > 0) ? ((double)(a->mrp - a->price) / a->mrp) : 0;
            double disc_b = (b->mrp > 0) ? ((double)(b->mrp - b->price) / b->mrp) : 0;
            if (disc_a > disc_b) return -1;
            if (disc_a < disc_b) return 1;
            return a->id - b->id;
        }

        case SORT_KEY_RELEVANCE:
        default:
            return a->id - b->id;
    }
}

/*
 * Internal recursive merge sort helper.
 * Time Complexity: O(n log n)
 * Space Complexity: O(n) auxiliary
 * Stable: Yes (maintains <= on left sub-array)
 */
static void merge_sort_internal(Product *arr, Product *temp, int left, int right, SortKey key, unsigned long long *comparisons) {
    if (left >= right) return;

    int mid = left + (right - left) / 2;
    merge_sort_internal(arr, temp, left, mid, key, comparisons);
    merge_sort_internal(arr, temp, mid + 1, right, key, comparisons);

    int i = left;
    int j = mid + 1;
    int k = left;

    while (i <= mid && j <= right) {
        if (compare_products(&arr[i], &arr[j], key, comparisons) <= 0) {
            temp[k++] = arr[i++];
        } else {
            temp[k++] = arr[j++];
        }
    }

    while (i <= mid) {
        temp[k++] = arr[i++];
    }

    while (j <= right) {
        temp[k++] = arr[j++];
    }

    for (int idx = left; idx <= right; idx++) {
        arr[idx] = temp[idx];
    }
}

void merge_sort_products(Product *arr, int n, SortKey key, unsigned long long *comparisons) {
    if (!arr || n <= 1) return;

    Product *temp = (Product *)malloc(sizeof(Product) * (size_t)n);
    if (!temp) return;

    merge_sort_internal(arr, temp, 0, n - 1, key, comparisons);
    free(temp);
}

/*
 * Internal helper to swap two product structs
 */
static void swap_products(Product *a, Product *b) {
    Product tmp = *a;
    *a = *b;
    *b = tmp;
}

/*
 * Partition helper with randomized pivot for Quick Sort.
 * Time Complexity: O(n)
 * Space Complexity: O(1)
 */
static int randomized_partition(Product *arr, int low, int high, SortKey key, unsigned long long *comparisons) {
    int pivot_idx = low + (rand() % (high - low + 1));
    swap_products(&arr[pivot_idx], &arr[high]);

    Product *pivot = &arr[high];
    int i = low - 1;

    for (int j = low; j < high; j++) {
        if (compare_products(&arr[j], pivot, key, comparisons) <= 0) {
            i++;
            swap_products(&arr[i], &arr[j]);
        }
    }
    swap_products(&arr[i + 1], &arr[high]);
    return i + 1;
}

/*
 * Recursive Quick Sort helper.
 * Time Complexity: O(n log n) average, O(n^2) worst case.
 * Space Complexity: O(log n) call stack.
 */
static void quick_sort_internal(Product *arr, int low, int high, SortKey key, unsigned long long *comparisons) {
    if (low < high) {
        int pi = randomized_partition(arr, low, high, key, comparisons);
        quick_sort_internal(arr, low, pi - 1, key, comparisons);
        quick_sort_internal(arr, pi + 1, high, key, comparisons);
    }
}

void quick_sort_products(Product *arr, int n, SortKey key, unsigned long long *comparisons) {
    if (!arr || n <= 1) return;
    quick_sort_internal(arr, 0, n - 1, key, comparisons);
}
