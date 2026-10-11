#include "cart.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_USER_CARTS 256

static UserCart g_carts[MAX_USER_CARTS];
static int g_cart_count = 0;

static Order g_orders[MAX_ORDERS];
static int g_order_count = 0;

static char g_carts_file[256];
static char g_wishlists_file[256];
static char g_orders_file[256];

UserCart *cart_get_for_user(const char *user_email) {
    if (!user_email || user_email[0] == '\0') {
        user_email = "guest@shopkart.local";
    }

    for (int i = 0; i < g_cart_count; i++) {
        if (strncmp(g_carts[i].user_email, user_email, sizeof(g_carts[i].user_email)) == 0) {
            return &g_carts[i];
        }
    }

    if (g_cart_count < MAX_USER_CARTS) {
        UserCart *c = &g_carts[g_cart_count++];
        memset(c, 0, sizeof(UserCart));
        snprintf(c->user_email, sizeof(c->user_email), "%s", user_email);
        stack_init(&c->undo_stack);
        linkedlist_init(&c->wishlist);
        queue_init(&c->recently_viewed);
        return c;
    }
    return NULL;
}

int cart_init(const char *carts_csv, const char *wishlists_csv, const char *orders_csv) {
    snprintf(g_carts_file, sizeof(g_carts_file), "%s", carts_csv ? carts_csv : "data/carts.csv");
    snprintf(g_wishlists_file, sizeof(g_wishlists_file), "%s", wishlists_csv ? wishlists_csv : "data/wishlists.csv");
    snprintf(g_orders_file, sizeof(g_orders_file), "%s", orders_csv ? orders_csv : "data/orders.csv");

    g_cart_count = 0;
    g_order_count = 0;

    /* Load Carts */
    FILE *fp = fopen(g_carts_file, "r");
    if (fp) {
        char line[512];
        /* Skip header if present */
        if (fgets(line, sizeof(line), fp)) {
            if (strstr(line, "user_email") == NULL) {
                fseek(fp, 0, SEEK_SET);
            }
        }
        while (fgets(line, sizeof(line), fp)) {
            char email[128] = {0};
            int pid = 0;
            int qty = 0;
            if (sscanf(line, "%127[^,],%d,%d", email, &pid, &qty) == 3 && pid > 0 && qty > 0) {
                cart_add_item(email, pid, qty);
            }
        }
        fclose(fp);
    }

    /* Load Wishlists */
    fp = fopen(g_wishlists_file, "r");
    if (fp) {
        char line[512];
        if (fgets(line, sizeof(line), fp)) {
            if (strstr(line, "user_email") == NULL) {
                fseek(fp, 0, SEEK_SET);
            }
        }
        while (fgets(line, sizeof(line), fp)) {
            char email[128] = {0};
            int pid = 0;
            if (sscanf(line, "%127[^,],%d", email, &pid) == 2 && pid > 0) {
                wishlist_add(email, pid);
            }
        }
        fclose(fp);
    }

    /* Load Orders */
    fp = fopen(g_orders_file, "r");
    if (fp) {
        char line[1024];
        if (fgets(line, sizeof(line), fp)) {
            if (strstr(line, "order_id") == NULL) {
                fseek(fp, 0, SEEK_SET);
            }
        }
        while (fgets(line, sizeof(line), fp) && g_order_count < MAX_ORDERS) {
            Order *o = &g_orders[g_order_count];
            memset(o, 0, sizeof(Order));
            int parsed = sscanf(line, "%31[^,],%127[^,],%d,%d,%d,%d,%255[^,],%31[^,],%31s",
                                o->order_id, o->user_email, &o->product_id,
                                &o->quantity, &o->price_each, &o->total_price,
                                o->delivery_address, o->payment_method, o->order_date);
            if (parsed == 9) {
                g_order_count++;
            }
        }
        fclose(fp);
    }

    return 1;
}

int cart_save_all(void) {
    /* Save Carts */
    FILE *fp = fopen(g_carts_file, "w");
    if (fp) {
        fprintf(fp, "user_email,product_id,quantity\n");
        for (int i = 0; i < g_cart_count; i++) {
            UserCart *c = &g_carts[i];
            for (int j = 0; j < c->item_count; j++) {
                fprintf(fp, "%s,%d,%d\n", c->user_email, c->items[j].product_id, c->items[j].quantity);
            }
        }
        fclose(fp);
    }

    /* Save Wishlists */
    fp = fopen(g_wishlists_file, "w");
    if (fp) {
        fprintf(fp, "user_email,product_id\n");
        for (int i = 0; i < g_cart_count; i++) {
            UserCart *c = &g_carts[i];
            int ids[MAX_PRODUCTS];
            int count = linkedlist_to_array(&c->wishlist, ids, MAX_PRODUCTS);
            for (int j = 0; j < count; j++) {
                fprintf(fp, "%s,%d\n", c->user_email, ids[j]);
            }
        }
        fclose(fp);
    }

    /* Save Orders */
    fp = fopen(g_orders_file, "w");
    if (fp) {
        fprintf(fp, "order_id,user_email,product_id,quantity,price_each,total_price,address,payment_method,order_date\n");
        for (int i = 0; i < g_order_count; i++) {
            Order *o = &g_orders[i];
            fprintf(fp, "%s,%s,%d,%d,%d,%d,%s,%s,%s\n",
                    o->order_id, o->user_email, o->product_id,
                    o->quantity, o->price_each, o->total_price,
                    o->delivery_address, o->payment_method, o->order_date);
        }
        fclose(fp);
    }

    return 1;
}

int cart_add_item(const char *user_email, int product_id, int quantity) {
    if (product_id <= 0 || quantity <= 0) return 0;
    UserCart *cart = cart_get_for_user(user_email);
    if (!cart) return 0;

    /* Check if already in cart */
    for (int i = 0; i < cart->item_count; i++) {
        if (cart->items[i].product_id == product_id) {
            cart->items[i].quantity += quantity;
            cart_save_all();
            return 1;
        }
    }

    if (cart->item_count < MAX_CART_ITEMS) {
        cart->items[cart->item_count].product_id = product_id;
        cart->items[cart->item_count].quantity = quantity;
        cart->item_count++;
        cart_save_all();
        return 1;
    }
    return 0;
}

int cart_update_quantity(const char *user_email, int product_id, int new_quantity) {
    if (product_id <= 0) return 0;
    if (new_quantity <= 0) {
        return cart_remove_item(user_email, product_id);
    }

    UserCart *cart = cart_get_for_user(user_email);
    if (!cart) return 0;

    for (int i = 0; i < cart->item_count; i++) {
        if (cart->items[i].product_id == product_id) {
            cart->items[i].quantity = new_quantity;
            cart_save_all();
            return 1;
        }
    }
    return 0;
}

int cart_remove_item(const char *user_email, int product_id) {
    if (product_id <= 0) return 0;
    UserCart *cart = cart_get_for_user(user_email);
    if (!cart) return 0;

    for (int i = 0; i < cart->item_count; i++) {
        if (cart->items[i].product_id == product_id) {
            const Product *p = product_get_by_id(product_id);
            if (p) {
                /* Push onto Undo Stack */
                stack_push(&cart->undo_stack, p, cart->items[i].quantity);
            }

            /* Shift remaining items down */
            for (int j = i; j < cart->item_count - 1; j++) {
                cart->items[j] = cart->items[j + 1];
            }
            cart->item_count--;
            cart_save_all();
            return 1;
        }
    }
    return 0;
}

int cart_undo_remove(const char *user_email) {
    UserCart *cart = cart_get_for_user(user_email);
    if (!cart) return 0;

    CartUndoItem item;
    if (stack_pop(&cart->undo_stack, &item)) {
        cart_add_item(user_email, item.product.id, item.quantity);
        return 1;
    }
    return 0;
}

void cart_clear(const char *user_email) {
    UserCart *cart = cart_get_for_user(user_email);
    if (!cart) return;
    cart->item_count = 0;
    cart_save_all();
}

int cart_get_total_quantity(const char *user_email) {
    UserCart *cart = cart_get_for_user(user_email);
    if (!cart) return 0;
    int total = 0;
    for (int i = 0; i < cart->item_count; i++) {
        total += cart->items[i].quantity;
    }
    return total;
}

int wishlist_add(const char *user_email, int product_id) {
    UserCart *cart = cart_get_for_user(user_email);
    if (!cart) return 0;
    int ok = linkedlist_append(&cart->wishlist, product_id);
    if (ok) cart_save_all();
    return ok;
}

int wishlist_remove(const char *user_email, int product_id) {
    UserCart *cart = cart_get_for_user(user_email);
    if (!cart) return 0;
    int ok = linkedlist_remove(&cart->wishlist, product_id);
    if (ok) cart_save_all();
    return ok;
}

const LinkedList *wishlist_get(const char *user_email) {
    UserCart *cart = cart_get_for_user(user_email);
    return cart ? &cart->wishlist : NULL;
}

void recently_viewed_add(const char *user_email, int product_id) {
    UserCart *cart = cart_get_for_user(user_email);
    if (cart) {
        queue_enqueue(&cart->recently_viewed, product_id);
    }
}

const CircularQueue *recently_viewed_get(const char *user_email) {
    UserCart *cart = cart_get_for_user(user_email);
    return cart ? &cart->recently_viewed : NULL;
}

int order_create(const char *user_email, const char *address, const char *payment_method, char out_order_id[32]) {
    if (!user_email || !address) return 0;
    UserCart *cart = cart_get_for_user(user_email);
    if (!cart || cart->item_count == 0) return 0;

    time_t now = time(NULL);
    char date_str[32];
    struct tm *tm_info = localtime(&now);
    if (tm_info) {
        strftime(date_str, sizeof(date_str), "%Y-%m-%d %H:%M", tm_info);
    } else {
        snprintf(date_str, sizeof(date_str), "2024-10-11");
    }

    /* Generate unique Order ID */
    char order_id[32];
    snprintf(order_id, sizeof(order_id), "OD%ld%04d", (long)now, rand() % 10000);
    if (out_order_id) {
        snprintf(out_order_id, 32, "%s", order_id);
    }

    /* Convert cart items into orders */
    for (int i = 0; i < cart->item_count && g_order_count < MAX_ORDERS; i++) {
        const Product *p = product_get_by_id(cart->items[i].product_id);
        if (!p) continue;

        Order *o = &g_orders[g_order_count++];
        snprintf(o->order_id, sizeof(o->order_id), "%s", order_id);
        snprintf(o->user_email, sizeof(o->user_email), "%s", user_email);
        o->product_id = p->id;
        o->quantity = cart->items[i].quantity;
        o->price_each = p->price;
        o->total_price = p->price * cart->items[i].quantity;
        snprintf(o->delivery_address, sizeof(o->delivery_address), "%s", address);
        snprintf(o->payment_method, sizeof(o->payment_method), "%s", payment_method ? payment_method : "COD");
        snprintf(o->order_date, sizeof(o->order_date), "%s", date_str);
    }

    /* Empty cart */
    cart->item_count = 0;
    cart_save_all();
    return 1;
}

int order_get_history(const char *user_email, Order *out_orders, int max_orders) {
    if (!user_email || !out_orders || max_orders <= 0) return 0;

    int count = 0;
    /* Return newest orders first (from end of g_orders) */
    for (int i = g_order_count - 1; i >= 0 && count < max_orders; i--) {
        if (strcmp(g_orders[i].user_email, user_email) == 0) {
            out_orders[count++] = g_orders[i];
        }
    }
    return count;
}
