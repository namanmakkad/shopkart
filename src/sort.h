#ifndef SORT_H
#define SORT_H

#include "product.h"

typedef enum {
    SORT_KEY_RELEVANCE = 0,
    SORT_KEY_PRICE_ASC,
    SORT_KEY_PRICE_DESC,
    SORT_KEY_RATING_DESC,
    SORT_KEY_DISCOUNT_DESC
} SortKey;

typedef enum {
    ALGO_MERGE_SORT = 0,
    ALGO_QUICK_SORT
} SortAlgo;

/*
 * Comparator function for products based on sort key.
 * Returns negative if a < b, positive if a > b, 0 if equal.
 * Increments *comparisons if non-NULL.
 */
int compare_products(const Product *a, const Product *b, SortKey key, unsigned long long *comparisons);

/*
 * Merge Sort implementation.
 * Time Complexity: O(n log n) best, average, and worst case.
 * Space Complexity: O(n) auxiliary memory for buffer.
 * Stable: Yes (preserves original order for equivalent elements).
 */
void merge_sort_products(Product *arr, int n, SortKey key, unsigned long long *comparisons);

/*
 * Quick Sort implementation with randomized pivot selection.
 * Time Complexity: O(n log n) average, O(n^2) worst case.
 * Space Complexity: O(log n) call stack.
 * Stable: No.
 */
void quick_sort_products(Product *arr, int n, SortKey key, unsigned long long *comparisons);

#endif /* SORT_H */
