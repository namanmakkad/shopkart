#ifndef CART_H
#define CART_H

#include "product.h"
#include "stack.h"
#include "linkedlist.h"
#include "queue.h"

#define MAX_CART_ITEMS 64
#define MAX_ORDERS 512
#define MAX_ADDRESS_LEN 256

typedef struct {
    int product_id;
    int quantity;
} CartItem;

typedef struct {
    char user_email[128];
    CartItem items[MAX_CART_ITEMS];
    int item_count;
    Stack undo_stack;           /* Cart undo remove stack */
    LinkedList wishlist;        /* User wishlist linked list */
    CircularQueue recently_viewed; /* Circular queue of viewed items */
} UserCart;

typedef struct {
    char order_id[32];
    char user_email[128];
    int product_id;
    int quantity;
    int price_each;
    int total_price;
    char delivery_address[MAX_ADDRESS_LEN];
    char payment_method[32];
    char order_date[32];
} Order;

/* Initialize Cart subsystem and load carts.csv, wishlists.csv, orders.csv */
int cart_init(const char *carts_csv, const char *wishlists_csv, const char *orders_csv);

/* Save carts, wishlists, and orders to CSV */
int cart_save_all(void);

/* Get or create cart session struct for user */
UserCart *cart_get_for_user(const char *user_email);

/* Add product to user's cart (or increment quantity if already present) */
int cart_add_item(const char *user_email, int product_id, int quantity);

/* Update quantity of item in cart (quantity == 0 removes it) */
int cart_update_quantity(const char *user_email, int product_id, int new_quantity);

/* Remove item from cart and push onto undo stack */
int cart_remove_item(const char *user_email, int product_id);

/* Undo the last item removal from cart using Stack */
int cart_undo_remove(const char *user_email);

/* Clear all items from cart */
void cart_clear(const char *user_email);

/* Total number of distinct items / quantities in cart */
int cart_get_total_quantity(const char *user_email);

/* Wishlist Operations */
int wishlist_add(const char *user_email, int product_id);
int wishlist_remove(const char *user_email, int product_id);
const LinkedList *wishlist_get(const char *user_email);

/* Recently Viewed Operations */
void recently_viewed_add(const char *user_email, int product_id);
const CircularQueue *recently_viewed_get(const char *user_email);

/* Checkout & Orders */
int order_create(const char *user_email, const char *address, const char *payment_method, char out_order_id[32]);
int order_get_history(const char *user_email, Order *out_orders, int max_orders);

#endif /* CART_H */
