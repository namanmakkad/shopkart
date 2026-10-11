#include "search.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* Case-insensitive substring match helper */
static int case_insensitive_contains(const char *haystack, const char *needle) {
    if (!haystack || !needle) return 0;
    if (needle[0] == '\0') return 1;

    size_t nlen = strlen(needle);
    size_t hlen = strlen(haystack);
    if (nlen > hlen) return 0;

    for (size_t i = 0; i <= hlen - nlen; i++) {
        size_t j = 0;
        while (j < nlen && tolower((unsigned char)haystack[i + j]) == tolower((unsigned char)needle[j])) {
            j++;
        }
        if (j == nlen) {
            return 1;
        }
    }
    return 0;
}

int linear_search_products(const Product *arr, int n, const char *query, Product *out_matches, int max_matches) {
    if (!arr || n <= 0 || !out_matches || max_matches <= 0) return 0;
    if (!query || query[0] == '\0') {
        int count = (n < max_matches) ? n : max_matches;
        for (int i = 0; i < count; i++) {
            out_matches[i] = arr[i];
        }
        return count;
    }

    int match_count = 0;
    for (int i = 0; i < n && match_count < max_matches; i++) {
        if (case_insensitive_contains(arr[i].name, query) ||
            case_insensitive_contains(arr[i].brand, query) ||
            case_insensitive_contains(arr[i].category, query) ||
            case_insensitive_contains(arr[i].subcategory, query)) {
            out_matches[match_count++] = arr[i];
        }
    }
    return match_count;
}

int binary_search_by_id(const Product *arr, int n, int target_id) {
    if (!arr || n <= 0) return -1;

    int low = 0;
    int high = n - 1;

    while (low <= high) {
        int mid = low + (high - low) / 2;
        if (arr[mid].id == target_id) {
            return mid;
        } else if (arr[mid].id < target_id) {
            low = mid + 1;
        } else {
            high = mid - 1;
        }
    }
    return -1;
}

int binary_search_price_lower_bound(const Product *arr, int n, int min_price) {
    if (!arr || n <= 0) return 0;

    int low = 0;
    int high = n;

    while (low < high) {
        int mid = low + (high - low) / 2;
        if (arr[mid].price >= min_price) {
            high = mid;
        } else {
            low = mid + 1;
        }
    }
    return low;
}

int binary_search_price_upper_bound(const Product *arr, int n, int max_price) {
    if (!arr || n <= 0) return 0;

    int low = 0;
    int high = n;

    while (low < high) {
        int mid = low + (high - low) / 2;
        if (arr[mid].price > max_price) {
            high = mid;
        } else {
            low = mid + 1;
        }
    }
    return low;
}

int binary_search_filter_price_range(const Product *price_sorted_arr, int n, int min_price, int max_price, Product *out_matches, int max_matches) {
    if (!price_sorted_arr || n <= 0 || !out_matches || max_matches <= 0) return 0;

    int lb = binary_search_price_lower_bound(price_sorted_arr, n, min_price);
    int ub = binary_search_price_upper_bound(price_sorted_arr, n, max_price);

    int count = 0;
    for (int i = lb; i < ub && count < max_matches; i++) {
        out_matches[count++] = price_sorted_arr[i];
    }
    return count;
}

/* ================= TRIE IMPLEMENTATION ================= */

static int char_to_trie_idx(char c) {
    c = (char)tolower((unsigned char)c);
    if (c >= 'a' && c <= 'z') return c - 'a';
    if (c >= '0' && c <= '9') return 26 + (c - '0');
    if (c == ' ') return 36;
    if (c == '-') return 37;
    return -1;
}

TrieNode *trie_create_node(void) {
    TrieNode *node = (TrieNode *)calloc(1, sizeof(TrieNode));
    if (!node) return NULL;
    node->is_end_of_word = 0;
    node->product_id = 0;
    node->original_word[0] = '\0';
    return node;
}

void trie_insert(TrieNode *root, const char *word, int product_id) {
    if (!root || !word || word[0] == '\0') return;

    TrieNode *curr = root;
    for (size_t i = 0; word[i] != '\0'; i++) {
        int idx = char_to_trie_idx(word[i]);
        if (idx < 0) continue;

        if (!curr->children[idx]) {
            curr->children[idx] = trie_create_node();
            if (!curr->children[idx]) return;
        }
        curr = curr->children[idx];
    }

    curr->is_end_of_word = 1;
    curr->product_id = product_id;
    snprintf(curr->original_word, sizeof(curr->original_word), "%s", word);
}

/* Recursive DFS helper to gather suggestions */
static void trie_dfs_collect(const TrieNode *node, char out_suggestions[][64], int *count, int max_suggestions) {
    if (!node || *count >= max_suggestions) return;

    if (node->is_end_of_word && node->original_word[0] != '\0') {
        /* Check for duplicates in output list */
        int exists = 0;
        for (int i = 0; i < *count; i++) {
            if (strcmp(out_suggestions[i], node->original_word) == 0) {
                exists = 1;
                break;
            }
        }
        if (!exists) {
            snprintf(out_suggestions[*count], sizeof(out_suggestions[*count]), "%s", node->original_word);
            (*count)++;
            if (*count >= max_suggestions) return;
        }
    }

    for (int i = 0; i < TRIE_ALPHABET_SIZE; i++) {
        if (node->children[i]) {
            trie_dfs_collect(node->children[i], out_suggestions, count, max_suggestions);
            if (*count >= max_suggestions) return;
        }
    }
}

int trie_get_suggestions(const TrieNode *root, const char *prefix, char out_suggestions[][64], int max_suggestions) {
    if (!root || !out_suggestions || max_suggestions <= 0) return 0;
    if (!prefix || prefix[0] == '\0') return 0;

    const TrieNode *curr = root;
    for (size_t i = 0; prefix[i] != '\0'; i++) {
        int idx = char_to_trie_idx(prefix[i]);
        if (idx < 0) continue;
        if (!curr->children[idx]) {
            return 0; /* No matching prefix */
        }
        curr = curr->children[idx];
    }

    int count = 0;
    trie_dfs_collect(curr, out_suggestions, &count, max_suggestions);
    return count;
}

void trie_free(TrieNode *root) {
    if (!root) return;
    for (int i = 0; i < TRIE_ALPHABET_SIZE; i++) {
        if (root->children[i]) {
            trie_free(root->children[i]);
            root->children[i] = NULL;
        }
    }
    free(root);
}
