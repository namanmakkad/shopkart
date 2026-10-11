#ifndef BST_H
#define BST_H

#include "product.h"

/*
 * Binary Search Tree Node ordered by product price.
 */
typedef struct BSTNode {
    int price;
    Product product;
    struct BSTNode *left;
    struct BSTNode *right;
} BSTNode;

/*
 * Binary Search Tree for Price-Ordered Index.
 * Time Complexity: Insert O(h), Search O(h), In-order traversal O(n).
 * Space Complexity: O(n) nodes, O(h) call stack recursion.
 */
typedef struct {
    BSTNode *root;
    int size;
} BST;

/* Initialize empty BST */
void bst_init(BST *tree);

/* Insert a product into BST ordered by price. Returns 1 on success, 0 on failure. */
int bst_insert(BST *tree, const Product *product);

/*
 * In-Order Traversal: yields products in ascending order of price.
 * Time Complexity: O(n).
 * Returns total count placed in out_arr.
 */
int bst_inorder_traversal(const BST *tree, Product *out_arr, int max_capacity);

/*
 * Range Query: finds all products with min_price <= price <= max_price using BST pruning.
 * Time Complexity: O(h + k) where h is height and k is count of matched items.
 * Returns count of matching items.
 */
int bst_range_search(const BST *tree, int min_price, int max_price, Product *out_arr, int max_capacity);

/* Free all nodes in BST */
void bst_free(BST *tree);

#endif /* BST_H */
