#include "product.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Product g_products[MAX_PRODUCTS];
static int g_product_count = 0;

/* Helper to parse a single CSV field supporting optional quotation marks */
static const char *parse_csv_field(const char *ptr, char *dst, size_t dst_size) {
    if (!ptr || !dst || dst_size == 0) return NULL;
    dst[0] = '\0';
    size_t d_idx = 0;

    while (*ptr == ' ' || *ptr == '\t') ptr++;

    if (*ptr == '"') {
        ptr++; /* skip opening quote */
        while (*ptr) {
            if (*ptr == '"') {
                if (*(ptr + 1) == '"') {
                    if (d_idx + 1 < dst_size) {
                        dst[d_idx++] = '"';
                    }
                    ptr += 2;
                } else {
                    ptr++; /* closing quote */
                    break;
                }
            } else {
                if (d_idx + 1 < dst_size) {
                    dst[d_idx++] = *ptr;
                }
                ptr++;
            }
        }
        while (*ptr && *ptr != ',' && *ptr != '\r' && *ptr != '\n') {
            ptr++;
        }
    } else {
        while (*ptr && *ptr != ',' && *ptr != '\r' && *ptr != '\n') {
            if (d_idx + 1 < dst_size) {
                dst[d_idx++] = *ptr;
            }
            ptr++;
        }
    }
    dst[d_idx] = '\0';

    if (*ptr == ',') {
        ptr++;
    }
    return ptr;
}

int product_load_all(const char *csv_path) {
    if (!csv_path) return -1;
    FILE *fp = fopen(csv_path, "r");
    if (!fp) {
        return -1;
    }

    char line[4096];
    g_product_count = 0;

    /* Read header line */
    if (!fgets(line, sizeof(line), fp)) {
        fclose(fp);
        return 0;
    }

    while (fgets(line, sizeof(line), fp) && g_product_count < MAX_PRODUCTS) {
        /* Strip line break */
        size_t len = strlen(line);
        while (len > 0 && (line[len - 1] == '\r' || line[len - 1] == '\n')) {
            line[--len] = '\0';
        }
        if (len == 0) continue;

        Product *p = &g_products[g_product_count];
        memset(p, 0, sizeof(Product));

        char f_id[32] = {0};
        char f_name[MAX_NAME_LEN] = {0};
        char f_brand[MAX_BRAND_LEN] = {0};
        char f_cat[MAX_CAT_LEN] = {0};
        char f_subcat[MAX_SUBCAT_LEN] = {0};
        char f_price[32] = {0};
        char f_mrp[32] = {0};
        char f_rating[32] = {0};
        char f_reviews[32] = {0};
        char f_stock[32] = {0};
        char f_img[MAX_IMG_LEN] = {0};
        char f_hl[MAX_HIGHLIGHTS_LEN] = {0};
        char f_specs[MAX_SPECS_LEN] = {0};

        const char *cur = line;
        cur = parse_csv_field(cur, f_id, sizeof(f_id));
        cur = parse_csv_field(cur, f_name, sizeof(f_name));
        cur = parse_csv_field(cur, f_brand, sizeof(f_brand));
        cur = parse_csv_field(cur, f_cat, sizeof(f_cat));
        cur = parse_csv_field(cur, f_subcat, sizeof(f_subcat));
        cur = parse_csv_field(cur, f_price, sizeof(f_price));
        cur = parse_csv_field(cur, f_mrp, sizeof(f_mrp));
        cur = parse_csv_field(cur, f_rating, sizeof(f_rating));
        cur = parse_csv_field(cur, f_reviews, sizeof(f_reviews));
        cur = parse_csv_field(cur, f_stock, sizeof(f_stock));
        cur = parse_csv_field(cur, f_img, sizeof(f_img));
        cur = parse_csv_field(cur, f_hl, sizeof(f_hl));
        cur = parse_csv_field(cur, f_specs, sizeof(f_specs));

        p->id = atoi(f_id);
        if (p->id <= 0) continue;

        snprintf(p->name, sizeof(p->name), "%s", f_name);
        snprintf(p->brand, sizeof(p->brand), "%s", f_brand);
        snprintf(p->category, sizeof(p->category), "%s", f_cat);
        snprintf(p->subcategory, sizeof(p->subcategory), "%s", f_subcat);
        p->price = atoi(f_price);
        p->mrp = atoi(f_mrp);
        p->rating = atof(f_rating);
        p->review_count = atoi(f_reviews);
        p->stock = atoi(f_stock);
        snprintf(p->image, sizeof(p->image), "%s", f_img);
        snprintf(p->highlights, sizeof(p->highlights), "%s", f_hl);
        snprintf(p->specs, sizeof(p->specs), "%s", f_specs);

        g_product_count++;
    }

    fclose(fp);
    return g_product_count;
}

int product_count_total(void) {
    return g_product_count;
}

const Product *product_get_all(void) {
    return g_products;
}

const Product *product_get_by_id(int id) {
    for (int i = 0; i < g_product_count; i++) {
        if (g_products[i].id == id) {
            return &g_products[i];
        }
    }
    return NULL;
}
