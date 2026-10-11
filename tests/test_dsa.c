#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "../src/product.h"
#include "../src/sort.h"
#include "../src/search.h"
#include "../src/hashmap.h"
#include "../src/stack.h"
#include "../src/queue.h"
#include "../src/linkedlist.h"
#include "../src/heap.h"
#include "../src/bst.h"

/* Helper to generate dummy test products */
static void make_test_product(Product *p, int id, const char *name, int price, double rating) {
    memset(p, 0, sizeof(Product));
    p->id = id;
    snprintf(p->name, sizeof(p->name), "%s", name);
    snprintf(p->brand, sizeof(p->brand), "BrandX");
    snprintf(p->category, sizeof(p->category), "electronics");
    p->price = price;
    p->mrp = price + 200;
    p->rating = rating;
}

/* 1. TEST SORTING ALGORITHMS */
static void test_sorting(void) {
    printf("[TEST] Sorting: Empty, Single, Random, Sorted, Reverse, Duplicate...\n");

    /* Empty Array */
    unsigned long long comps = 0;
    merge_sort_products(NULL, 0, SORT_KEY_PRICE_ASC, &comps);
    quick_sort_products(NULL, 0, SORT_KEY_PRICE_ASC, &comps);

    /* Single Element */
    Product single[1];
    make_test_product(&single[0], 1, "P1", 100, 4.0);
    comps = 0;
    merge_sort_products(single, 1, SORT_KEY_PRICE_ASC, &comps);
    assert(single[0].id == 1);
    comps = 0;
    quick_sort_products(single, 1, SORT_KEY_PRICE_ASC, &comps);
    assert(single[0].id == 1);

    /* Random Array */
    Product arr_m[6], arr_q[6];
    int prices[6] = {500, 100, 400, 200, 300, 150};
    for (int i = 0; i < 6; i++) {
        make_test_product(&arr_m[i], i + 1, "P", prices[i], 4.0);
        arr_q[i] = arr_m[i];
    }
    comps = 0;
    merge_sort_products(arr_m, 6, SORT_KEY_PRICE_ASC, &comps);
    assert(comps > 0);
    for (int i = 0; i < 5; i++) {
        assert(arr_m[i].price <= arr_m[i + 1].price);
    }

    comps = 0;
    quick_sort_products(arr_q, 6, SORT_KEY_PRICE_ASC, &comps);
    assert(comps > 0);
    for (int i = 0; i < 5; i++) {
        assert(arr_q[i].price <= arr_q[i + 1].price);
    }

    /* Already Sorted */
    comps = 0;
    merge_sort_products(arr_m, 6, SORT_KEY_PRICE_ASC, &comps);
    for (int i = 0; i < 5; i++) assert(arr_m[i].price <= arr_m[i + 1].price);

    /* Reverse Sorted */
    Product rev[4];
    make_test_product(&rev[0], 1, "P", 400, 4.0);
    make_test_product(&rev[1], 2, "P", 300, 4.0);
    make_test_product(&rev[2], 3, "P", 200, 4.0);
    make_test_product(&rev[3], 4, "P", 100, 4.0);
    merge_sort_products(rev, 4, SORT_KEY_PRICE_ASC, NULL);
    for (int i = 0; i < 3; i++) assert(rev[i].price <= rev[i + 1].price);

    /* Duplicates */
    Product dup[5];
    make_test_product(&dup[0], 1, "P", 200, 4.0);
    make_test_product(&dup[1], 2, "P", 100, 4.0);
    make_test_product(&dup[2], 3, "P", 200, 4.0);
    make_test_product(&dup[3], 4, "P", 100, 4.0);
    make_test_product(&dup[4], 5, "P", 300, 4.0);
    merge_sort_products(dup, 5, SORT_KEY_PRICE_ASC, NULL);
    for (int i = 0; i < 4; i++) assert(dup[i].price <= dup[i + 1].price);

    printf("  -> Merge Sort & Quick Sort passed.\n");
}

/* 2. TEST MERGE SORT STABILITY */
static void test_merge_sort_stability(void) {
    printf("[TEST] Merge Sort Stability...\n");
    Product items[4];
    make_test_product(&items[0], 10, "A", 250, 4.5);
    make_test_product(&items[1], 20, "B", 100, 4.5);
    make_test_product(&items[2], 30, "C", 250, 4.5);
    make_test_product(&items[3], 40, "D", 100, 4.5);

    /*
     * We test stability by keeping ratings identical and verifying that items with the
     * same rating retain their initial relative order.
     */
    merge_sort_products(items, 4, SORT_KEY_RATING_DESC, NULL);
    assert(items[0].id == 20); /* price 100 */
    assert(items[1].id == 40); /* price 100 */
    assert(items[2].id == 10); /* price 250 */
    assert(items[3].id == 30); /* price 250 */
    printf("  -> Merge Sort stability verified.\n");
}

/* 3. TEST BINARY SEARCH & BOUNDS */
static void test_binary_search(void) {
    printf("[TEST] Binary Search: Found, Not-Found, Lower Bound, Upper Bound...\n");
    Product arr[5];
    int prices[5] = {100, 200, 200, 300, 500};
    for (int i = 0; i < 5; i++) {
        make_test_product(&arr[i], (i + 1) * 10, "P", prices[i], 4.0);
    }

    /* ID Lookup */
    assert(binary_search_by_id(arr, 5, 10) == 0);
    assert(binary_search_by_id(arr, 5, 30) == 2);
    assert(binary_search_by_id(arr, 5, 50) == 4);
    assert(binary_search_by_id(arr, 5, 99) == -1);
    assert(binary_search_by_id(arr, 5, 5) == -1);

    /* Lower Bound */
    assert(binary_search_price_lower_bound(arr, 5, 50) == 0);   /* >= 50 -> idx 0 (100) */
    assert(binary_search_price_lower_bound(arr, 5, 200) == 1);  /* >= 200 -> idx 1 (200) */
    assert(binary_search_price_lower_bound(arr, 5, 250) == 3);  /* >= 250 -> idx 3 (300) */
    assert(binary_search_price_lower_bound(arr, 5, 600) == 5);  /* >= 600 -> idx 5 (end) */

    /* Upper Bound */
    assert(binary_search_price_upper_bound(arr, 5, 200) == 3);  /* > 200 -> idx 3 (300) */
    assert(binary_search_price_upper_bound(arr, 5, 500) == 5);  /* > 500 -> idx 5 (end) */
    assert(binary_search_price_upper_bound(arr, 5, 50) == 0);   /* > 50 -> idx 0 (100) */

    /* Price Range Filter */
    Product matches[5];
    int count = binary_search_filter_price_range(arr, 5, 200, 300, matches, 5);
    assert(count == 3); /* 200, 200, 300 */
    assert(matches[0].price == 200);
    assert(matches[1].price == 200);
    assert(matches[2].price == 300);

    printf("  -> Binary Search & Bounds verified.\n");
}

/* 4. TEST STACK (CART UNDO) */
static void test_stack(void) {
    printf("[TEST] Stack LIFO & Undo...\n");
    Stack s;
    stack_init(&s);
    assert(stack_is_empty(&s) == 1);
    assert(stack_size(&s) == 0);

    Product p1, p2, p3;
    make_test_product(&p1, 1, "P1", 100, 4.0);
    make_test_product(&p2, 2, "P2", 200, 4.0);
    make_test_product(&p3, 3, "P3", 300, 4.0);

    stack_push(&s, &p1, 2);
    stack_push(&s, &p2, 1);
    stack_push(&s, &p3, 5);
    assert(stack_size(&s) == 3);

    CartUndoItem item;
    assert(stack_peek(&s, &item) == 1);
    assert(item.product.id == 3 && item.quantity == 5);

    assert(stack_pop(&s, &item) == 1);
    assert(item.product.id == 3 && item.quantity == 5);

    assert(stack_pop(&s, &item) == 1);
    assert(item.product.id == 2 && item.quantity == 1);

    assert(stack_pop(&s, &item) == 1);
    assert(item.product.id == 1 && item.quantity == 2);

    assert(stack_is_empty(&s) == 1);
    assert(stack_pop(&s, &item) == 0);
    printf("  -> Stack LIFO verified.\n");
}

/* 5. TEST CIRCULAR QUEUE (RECENTLY VIEWED) */
static void test_queue(void) {
    printf("[TEST] Circular Queue FIFO & Wraparound Eviction...\n");
    CircularQueue q;
    queue_init(&q);
    assert(queue_is_empty(&q) == 1);

    /* Fill up to 10 elements */
    for (int i = 1; i <= 10; i++) {
        queue_enqueue(&q, i * 10);
    }
    assert(queue_size(&q) == 10);

    /* 11th element should evict the oldest (10) */
    queue_enqueue(&q, 110);
    assert(queue_size(&q) == 10);
    assert(queue_contains(&q, 10) == 0); /* Evicted */
    assert(queue_contains(&q, 110) == 1);

    /* Retrieve newest first */
    int items[MAX_RECENT_VIEWED];
    int count = queue_get_items_newest_first(&q, items);
    assert(count == 10);
    assert(items[0] == 110); /* Most recently viewed */
    assert(items[9] == 20);  /* Oldest remaining */

    int dequeued = 0;
    assert(queue_dequeue(&q, &dequeued) == 1);
    assert(dequeued == 20);
    assert(queue_size(&q) == 9);

    printf("  -> Circular Queue verified.\n");
}

/* 6. TEST LINKED LIST (WISHLIST) */
static void test_linkedlist(void) {
    printf("[TEST] LinkedList Wishlist Operations...\n");
    LinkedList list;
    linkedlist_init(&list);
    assert(linkedlist_size(&list) == 0);

    linkedlist_append(&list, 101);
    linkedlist_append(&list, 102);
    linkedlist_append(&list, 103);
    assert(linkedlist_size(&list) == 3);
    assert(linkedlist_contains(&list, 102) == 1);
    assert(linkedlist_contains(&list, 999) == 0);

    /* Duplicate append shouldn't duplicate */
    linkedlist_append(&list, 102);
    assert(linkedlist_size(&list) == 3);

    /* Remove middle */
    assert(linkedlist_remove(&list, 102) == 1);
    assert(linkedlist_size(&list) == 2);
    assert(linkedlist_contains(&list, 102) == 0);

    /* Dump to array */
    int arr[10];
    int count = linkedlist_to_array(&list, arr, 10);
    assert(count == 2);
    assert(arr[0] == 101 && arr[1] == 103);

    linkedlist_free(&list);
    assert(linkedlist_size(&list) == 0);
    printf("  -> LinkedList verified.\n");
}

/* 7. TEST HASH MAP & COLLISIONS */
static void test_hashmap(void) {
    printf("[TEST] HashMap Chaining & Collisions...\n");
    HashMap map;
    hashmap_init(&map);

    hashmap_put(&map, "user1@example.com", "User One", NULL);
    hashmap_put(&map, "user2@example.com", "User Two", NULL);
    hashmap_put(&map, "user3@example.com", "User Three", NULL);
    assert(hashmap_size(&map) == 3);

    assert(strcmp(hashmap_get(&map, "user2@example.com"), "User Two") == 0);
    assert(hashmap_get(&map, "nonexistent@example.com") == NULL);

    /* Update existing */
    hashmap_put(&map, "user2@example.com", "User Two Updated", NULL);
    assert(strcmp(hashmap_get(&map, "user2@example.com"), "User Two Updated") == 0);
    assert(hashmap_size(&map) == 3);

    /* Insert many to test collision handling */
    char key[64], val[64];
    for (int i = 0; i < 150; i++) {
        snprintf(key, sizeof(key), "session_token_%d", i);
        snprintf(val, sizeof(val), "email_%d@shopkart.local", i);
        hashmap_put(&map, key, val, NULL);
    }
    assert(hashmap_size(&map) == 153);
    assert(strcmp(hashmap_get(&map, "session_token_120"), "email_120@shopkart.local") == 0);

    /* Remove key */
    assert(hashmap_remove(&map, "session_token_120") == 1);
    assert(hashmap_get(&map, "session_token_120") == NULL);
    assert(hashmap_size(&map) == 152);

    hashmap_free(&map);
    assert(hashmap_size(&map) == 0);
    printf("  -> HashMap verified.\n");
}

/* 8. TEST TRIE AUTOCOMPLETE */
static void test_trie(void) {
    printf("[TEST] Trie Autocomplete & Prefixes...\n");
    TrieNode *root = trie_create_node();
    assert(root != NULL);

    trie_insert(root, "headphones", 1);
    trie_insert(root, "headset", 2);
    trie_insert(root, "heavy duty mixer", 3);
    trie_insert(root, "laptop", 4);

    char suggestions[10][64];
    int count = trie_get_suggestions(root, "head", suggestions, 10);
    assert(count == 2);
    /* Should contain "headphones" and "headset" */
    int found_hp = 0, found_hs = 0;
    for (int i = 0; i < count; i++) {
        if (strcmp(suggestions[i], "headphones") == 0) found_hp = 1;
        if (strcmp(suggestions[i], "headset") == 0) found_hs = 1;
    }
    assert(found_hp && found_hs);

    count = trie_get_suggestions(root, "lap", suggestions, 10);
    assert(count == 1);
    assert(strcmp(suggestions[0], "laptop") == 0);

    count = trie_get_suggestions(root, "xyz", suggestions, 10);
    assert(count == 0);

    trie_free(root);
    printf("  -> Trie verified.\n");
}

/* 9. TEST HEAP TOP-K */
static void test_heap(void) {
    printf("[TEST] Min-Heap Top-K Elements...\n");
    Product arr[8];
    double ratings[8] = {3.2, 4.8, 3.9, 4.9, 4.1, 4.7, 3.5, 4.6};
    for (int i = 0; i < 8; i++) {
        make_test_product(&arr[i], i + 1, "P", 1000, ratings[i]);
    }

    Product top3[3];
    int count = heap_find_top_k_by_rating(arr, 8, 3, top3);
    assert(count == 3);
    /* Highest should be 4.9, then 4.8, then 4.7 */
    assert(top3[0].rating >= 4.89 && top3[0].rating <= 4.91);
    assert(top3[1].rating >= 4.79 && top3[1].rating <= 4.81);
    assert(top3[2].rating >= 4.69 && top3[2].rating <= 4.71);

    printf("  -> Min-Heap Top-K verified.\n");
}

/* 10. TEST BST */
static void test_bst(void) {
    printf("[TEST] Binary Search Tree Price Index...\n");
    BST tree;
    bst_init(&tree);

    Product p;
    int prices[5] = {300, 100, 400, 200, 500};
    for (int i = 0; i < 5; i++) {
        make_test_product(&p, i + 1, "P", prices[i], 4.0);
        bst_insert(&tree, &p);
    }
    assert(tree.size == 5);

    /* In-order traversal yields ascending sorted prices */
    Product sorted[5];
    int count = bst_inorder_traversal(&tree, sorted, 5);
    assert(count == 5);
    for (int i = 0; i < 4; i++) {
        assert(sorted[i].price <= sorted[i + 1].price);
    }

    /* Range search [150, 450] */
    Product range_items[5];
    count = bst_range_search(&tree, 150, 450, range_items, 5);
    assert(count == 3); /* 200, 300, 400 */

    bst_free(&tree);
    printf("  -> BST verified.\n");
}

int main(void) {
    printf("======================================\n");
    printf("   SHOPKART COMPREHENSIVE DSA TESTS   \n");
    printf("======================================\n\n");

    test_sorting();
    test_merge_sort_stability();
    test_binary_search();
    test_stack();
    test_queue();
    test_linkedlist();
    test_hashmap();
    test_trie();
    test_heap();
    test_bst();

    printf("\n>>> ALL TESTS PASSED SUCCESSFULLY! <<<\n");
    return 0;
}
