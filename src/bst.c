#include "bst.h"
#include <stdlib.h>

void bst_init(BST *tree) {
    if (!tree) return;
    tree->root = NULL;
    tree->size = 0;
}

static BSTNode *bst_create_node(const Product *product) {
    BSTNode *node = (BSTNode *)malloc(sizeof(BSTNode));
    if (!node) return NULL;
    node->price = product->price;
    node->product = *product;
    node->left = NULL;
    node->right = NULL;
    return node;
}

static BSTNode *bst_insert_node(BSTNode *root, const Product *product, int *success) {
    if (!root) {
        BSTNode *n = bst_create_node(product);
        if (n) *success = 1;
        return n;
    }

    if (product->price <= root->price) {
        root->left = bst_insert_node(root->left, product, success);
    } else {
        root->right = bst_insert_node(root->right, product, success);
    }
    return root;
}

int bst_insert(BST *tree, const Product *product) {
    if (!tree || !product) return 0;
    int success = 0;
    tree->root = bst_insert_node(tree->root, product, &success);
    if (success) {
        tree->size++;
    }
    return success;
}

static void bst_inorder_helper(const BSTNode *root, Product *out_arr, int *count, int max_capacity) {
    if (!root || *count >= max_capacity) return;

    bst_inorder_helper(root->left, out_arr, count, max_capacity);
    if (*count < max_capacity) {
        out_arr[(*count)++] = root->product;
    }
    bst_inorder_helper(root->right, out_arr, count, max_capacity);
}

int bst_inorder_traversal(const BST *tree, Product *out_arr, int max_capacity) {
    if (!tree || !out_arr || max_capacity <= 0) return 0;
    int count = 0;
    bst_inorder_helper(tree->root, out_arr, &count, max_capacity);
    return count;
}

static void bst_range_helper(const BSTNode *root, int min_price, int max_price, Product *out_arr, int *count, int max_capacity) {
    if (!root || *count >= max_capacity) return;

    /* If current price > min_price, there could be valid items in left subtree */
    if (root->price > min_price) {
        bst_range_helper(root->left, min_price, max_price, out_arr, count, max_capacity);
    }

    /* Check current node */
    if (root->price >= min_price && root->price <= max_price) {
        if (*count < max_capacity) {
            out_arr[(*count)++] = root->product;
        }
    }

    /* If current price < max_price, there could be valid items in right subtree */
    if (root->price < max_price) {
        bst_range_helper(root->right, min_price, max_price, out_arr, count, max_capacity);
    }
}

int bst_range_search(const BST *tree, int min_price, int max_price, Product *out_arr, int max_capacity) {
    if (!tree || !out_arr || max_capacity <= 0) return 0;
    int count = 0;
    bst_range_helper(tree->root, min_price, max_price, out_arr, &count, max_capacity);
    return count;
}

static void bst_free_helper(BSTNode *root) {
    if (!root) return;
    bst_free_helper(root->left);
    bst_free_helper(root->right);
    free(root);
}

void bst_free(BST *tree) {
    if (!tree) return;
    bst_free_helper(tree->root);
    tree->root = NULL;
    tree->size = 0;
}
