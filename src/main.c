#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "server.h"
#include "product.h"
#include "auth.h"
#include "cart.h"

static volatile int running = 1;

static void signal_handler(int sig) {
    (void)sig;
    running = 0;
    printf("\nShutting down ShopKart server...\n");
}

int main(void) {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    printf("====================================================\n");
    printf("   ShopKart - Pure C E-Commerce & DSA Engine        \n");
    printf("====================================================\n");

    int prods = product_load_all("data/products.csv");
    printf("[DATA] Loaded %d products into memory from data/products.csv\n", prods);

    int users = auth_init("data/users.csv");
    printf("[AUTH] Initialized auth subsystem with %d users\n", users);

    cart_init("data/carts.csv", "data/wishlists.csv", "data/orders.csv");
    printf("[CART] Loaded carts, wishlists, and orders\n");

    if (server_init(SERVER_PORT) != 0) {
        fprintf(stderr, "ERROR: Failed to initialize server on port %d\n", SERVER_PORT);
        return 1;
    }

    server_run();
    server_cleanup();

    printf("ShopKart server stopped.\n");
    return 0;
}
