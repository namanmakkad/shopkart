#ifndef SEARCH_H
#define SEARCH_H

#include "product.h"

/*
 * Linear Substring Search:
 * Searches products where query matches case-insensitively in name, brand, or category.
 * Time Complexity: O(n * m) where n is product count, m is string length.
 * Space Complexity: O(1) auxiliary.
 * Returns count of matching products placed into out_matches.
 */
int linear_search_products(const Product *arr, int n, const char *query, Product *out_matches, int max_matches);

/*
 * Binary Search: Lookup Product by ID in an ID-sorted array.
 * Time Complexity: O(log n).
 * Space Complexity: O(1).
 * Returns index if found, or -1 if not found.
 */
int binary_search_by_id(const Product *arr, int n, int target_id);

/*
 * Binary Search: Find lower bound index in a price-sorted array.
 * Returns first index with product->price >= min_price.
 * Time Complexity: O(log n).
 * Space Complexity: O(1).
 */
int binary_search_price_lower_bound(const Product *arr, int n, int min_price);

/*
 * Binary Search: Find upper bound index in a price-sorted array.
 * Returns first index with product->price > max_price.
 * Time Complexity: O(log n).
 * Space Complexity: O(1).
 */
int binary_search_price_upper_bound(const Product *arr, int n, int max_price);

/*
 * Binary Search Price Range Filter:
 * Uses lower_bound and upper_bound on a price-sorted index to find products in [min_price, max_price].
 * Time Complexity: O(log n + k) where k is the number of matching elements.
 * Space Complexity: O(1).
 */
int binary_search_filter_price_range(const Product *price_sorted_arr, int n, int min_price, int max_price, Product *out_matches, int max_matches);

/*
 * Trie Data Structure for Prefix Autocomplete Suggestions.
 */
#define TRIE_ALPHABET_SIZE 38 /* a-z (26), 0-9 (10), space (1), dash (1) */

typedef struct TrieNode {
    struct TrieNode *children[TRIE_ALPHABET_SIZE];
    int is_end_of_word;
    int product_id;
    char original_word[64];
} TrieNode;

/* Create a new Trie node */
TrieNode *trie_create_node(void);

/* Insert a word into the Trie with its product ID */
void trie_insert(TrieNode *root, const char *word, int product_id);

/*
 * Prefix Autocomplete Query:
 * Returns up to max_suggestions matching phrases starting with prefix.
 * Time Complexity: O(p + s) where p is prefix length and s is size of subtree searched.
 * Space Complexity: O(1) auxiliary stack.
 */
int trie_get_suggestions(const TrieNode *root, const char *prefix, char out_suggestions[][64], int max_suggestions);

/* Free all nodes in the Trie */
void trie_free(TrieNode *root);

#endif /* SEARCH_H */
