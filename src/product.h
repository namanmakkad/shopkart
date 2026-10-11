#ifndef PRODUCT_H
#define PRODUCT_H

#include <stddef.h>

#define MAX_PRODUCTS 512
#define MAX_NAME_LEN 256
#define MAX_BRAND_LEN 64
#define MAX_CAT_LEN 64
#define MAX_SUBCAT_LEN 64
#define MAX_IMG_LEN 64
#define MAX_HIGHLIGHTS_LEN 512
#define MAX_SPECS_LEN 1024

typedef struct {
    int id;
    char name[MAX_NAME_LEN];
    char brand[MAX_BRAND_LEN];
    char category[MAX_CAT_LEN];
    char subcategory[MAX_SUBCAT_LEN];
    int price;
    int mrp;
    double rating;
    int review_count;
    int stock;
    char image[MAX_IMG_LEN];
    char highlights[MAX_HIGHLIGHTS_LEN];
    char specs[MAX_SPECS_LEN];
} Product;

/* Initialize and load products from CSV. Returns count loaded or -1 on error. */
int product_load_all(const char *csv_path);

/* Get total number of loaded products */
int product_count_total(void);

/* Get direct read-only pointer to array of all products */
const Product *product_get_all(void);

/* Find product by ID in O(1) or O(log n) */
const Product *product_get_by_id(int id);

#endif /* PRODUCT_H */
