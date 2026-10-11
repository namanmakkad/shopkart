#include "server.h"
#include "render.h"
#include "http_utils.h"
#include "product.h"
#include "sort.h"
#include "search.h"
#include "auth.h"
#include "cart.h"
#include "heap.h"
#include "bst.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#ifdef _WIN32
  #include <windows.h>
#endif

static socket_t g_server_fd = INVALID_SOCK;

/* Case-insensitive string comparison */
static int str_case_cmp(const char *s1, const char *s2) {
    if (!s1 || !s2) return s1 == s2 ? 0 : (s1 ? 1 : -1);
    while (*s1 && *s2) {
        int c1 = tolower((unsigned char)*s1);
        int c2 = tolower((unsigned char)*s2);
        if (c1 != c2) return c1 - c2;
        s1++;
        s2++;
    }
    return tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
}

/* High-resolution timer helper in milliseconds */
static double get_time_ms(void) {
#ifdef _WIN32
    LARGE_INTEGER freq, counter;
    if (QueryPerformanceFrequency(&freq) && QueryPerformanceCounter(&counter)) {
        return ((double)counter.QuadPart * 1000.0) / (double)freq.QuadPart;
    }
    return (double)clock() * 1000.0 / (double)CLOCKS_PER_SEC;
#else
    return (double)clock() * 1000.0 / (double)CLOCKS_PER_SEC;
#endif
}

void response_init(HttpResponse *resp) {
    if (!resp) return;
    memset(resp, 0, sizeof(HttpResponse));
    resp->status_code = 200;
    snprintf(resp->status_text, sizeof(resp->status_text), "OK");
    snprintf(resp->content_type, sizeof(resp->content_type), "text/html; charset=utf-8");
    resp->body = NULL;
    resp->body_length = 0;
    resp->extra_headers[0] = '\0';
}

void response_set_status(HttpResponse *resp, int code, const char *text) {
    if (!resp) return;
    resp->status_code = code;
    snprintf(resp->status_text, sizeof(resp->status_text), "%s", text ? text : "");
}

void response_set_body(HttpResponse *resp, const char *body, size_t length) {
    if (!resp) return;
    if (resp->body) {
        free(resp->body);
        resp->body = NULL;
    }
    resp->body_length = length;
    if (length > 0 && body) {
        resp->body = (char *)malloc(length + 1);
        if (resp->body) {
            memcpy(resp->body, body, length);
            resp->body[length] = '\0';
        } else {
            resp->body_length = 0;
        }
    } else {
        resp->body = (char *)malloc(1);
        if (resp->body) {
            resp->body[0] = '\0';
        }
    }
}

void response_set_html(HttpResponse *resp, const char *html) {
    if (!resp) return;
    snprintf(resp->content_type, sizeof(resp->content_type), "text/html; charset=utf-8");
    size_t len = html ? strlen(html) : 0;
    response_set_body(resp, html ? html : "", len);
}

void response_set_redirect(HttpResponse *resp, const char *url) {
    if (!resp) return;
    response_set_status(resp, 302, "Found");
    char header_line[512];
    snprintf(header_line, sizeof(header_line), "Location: %s\r\n", url ? url : "/");
    response_add_header(resp, header_line);
    response_set_body(resp, "", 0);
}

void response_add_header(HttpResponse *resp, const char *header_line) {
    if (!resp || !header_line) return;
    size_t current_len = strlen(resp->extra_headers);
    size_t add_len = strlen(header_line);
    if (current_len + add_len + 3 < sizeof(resp->extra_headers)) {
        snprintf(resp->extra_headers + current_len,
                 sizeof(resp->extra_headers) - current_len,
                 "%s%s",
                 header_line,
                 (add_len >= 2 && header_line[add_len - 2] == '\r' && header_line[add_len - 1] == '\n') ? "" : "\r\n");
    }
}

void response_free(HttpResponse *resp) {
    if (!resp) return;
    if (resp->body) {
        free(resp->body);
        resp->body = NULL;
    }
    resp->body_length = 0;
}

const char *request_get_header(const HttpRequest *req, const char *name) {
    if (!req || !name) return NULL;
    for (int i = 0; i < req->header_count; i++) {
        if (str_case_cmp(req->headers[i].name, name) == 0) {
            return req->headers[i].value;
        }
    }
    return NULL;
}

static void extract_session_token(HttpRequest *req) {
    if (!req) return;
    req->session_token[0] = '\0';
    const char *cookie_hdr = request_get_header(req, "Cookie");
    if (!cookie_hdr) return;

    const char *cookie_name = "shopkart_session=";
    size_t cookie_name_len = strlen(cookie_name);
    const char *p = cookie_hdr;

    while (*p != '\0') {
        while (*p == ' ' || *p == ';') p++;
        if (strncmp(p, cookie_name, cookie_name_len) == 0) {
            const char *val_start = p + cookie_name_len;
            const char *val_end = strchr(val_start, ';');
            size_t token_len = val_end ? (size_t)(val_end - val_start) : strlen(val_start);
            if (token_len >= sizeof(req->session_token)) {
                token_len = sizeof(req->session_token) - 1;
            }
            memcpy(req->session_token, val_start, token_len);
            req->session_token[token_len] = '\0';
            return;
        }
        p = strchr(p, ';');
        if (!p) break;
    }
}

/* Helper to setup common header and nav template variables */
static void setup_common_vars(const HttpRequest *req, TemplateContext *ctx, const char *active_search) {
    const User *u = auth_get_user_by_session(req->session_token);
    const char *email = u ? u->email : "guest@shopkart.local";

    if (u) {
        char greeting[128];
        snprintf(greeting, sizeof(greeting), "Hi, %s", u->name);
        template_set(ctx, "user_greeting", greeting);
        template_set(ctx, "is_logged_in", "1");
        template_set(ctx, "user_email", u->email);
        template_set(ctx, "user_name", u->name);
    } else {
        template_set(ctx, "user_greeting", "Login");
        template_set(ctx, "is_logged_in", "0");
        template_set(ctx, "user_email", "guest@shopkart.local");
        template_set(ctx, "user_name", "Guest");
    }

    int cart_qty = cart_get_total_quantity(email);
    char qty_str[32];
    snprintf(qty_str, sizeof(qty_str), "%d", cart_qty);
    template_set(ctx, "cart_count", qty_str);

    template_set(ctx, "search_query", active_search ? active_search : "");
}

/* Helper to build a single product card HTML */
static void render_product_card_html(const Product *p, char *out, size_t out_size) {
    if (!p || !out || out_size == 0) return;

    int discount_pct = 0;
    if (p->mrp > 0) {
        discount_pct = (int)(((p->mrp - p->price) * 100) / p->mrp);
    }

    /* Assign icon based on category */
    const char *icon = "&#128717;";
    if (strcmp(p->category, "electronics") == 0) icon = "&#127911;";
    else if (strcmp(p->category, "mobiles") == 0) icon = "&#128241;";
    else if (strcmp(p->category, "fashion") == 0) icon = "&#128084;";
    else if (strcmp(p->category, "home") == 0) icon = "&#128715;";
    else if (strcmp(p->category, "appliances") == 0) icon = "&#129482;";
    else if (strcmp(p->category, "food") == 0) icon = "&#127871;";
    else if (strcmp(p->category, "beauty") == 0) icon = "&#10024;";
    else if (strcmp(p->category, "toys") == 0) icon = "&#129528;";
    else if (strcmp(p->category, "sports") == 0) icon = "&#127951;";
    else if (strcmp(p->category, "books") == 0) icon = "&#128214;";

    snprintf(out, out_size,
        "<a href=\"/product?id=%d\" class=\"product-card\">\n"
        "  <div class=\"card-img-placeholder bg-%c%c%c%c\">\n"
        "    <span>%s</span>\n"
        "  </div>\n"
        "  <div class=\"card-title\" title=\"%s\">%s</div>\n"
        "  <div class=\"rating-badge\">%.1f &#9733;</div>\n"
        "  <div class=\"price-row\">\n"
        "    <span class=\"current-price\">&#8377;%d</span>\n"
        "    <span class=\"original-price\">&#8377;%d</span>\n"
        "    <span class=\"discount-tag\">%d%% off</span>\n"
        "  </div>\n"
        "</a>\n",
        p->id,
        p->category[0], p->category[1], p->category[2], p->category[3],
        icon,
        p->name, p->name,
        p->rating,
        p->price,
        p->mrp,
        discount_pct);
}

/* ================= 404 HANDLER ================= */
static void handle_404(const HttpRequest *req, HttpResponse *resp) {
    TemplateContext ctx;
    template_context_init(&ctx);
    setup_common_vars(req, &ctx, "");
    template_set(&ctx, "page_title", "Page Not Found - ShopKart");

    char *html = render_template("404.html", &ctx);
    if (html) {
        response_set_status(resp, 404, "Not Found");
        response_set_html(resp, html);
        free(html);
    } else {
        response_set_status(resp, 404, "Not Found");
        response_set_html(resp, "<h1>404 Not Found</h1>");
    }
    template_context_free(&ctx);
}

/* ================= HOME PAGE HANDLER ================= */
static void handle_home(const HttpRequest *req, HttpResponse *resp) {
    TemplateContext ctx;
    template_context_init(&ctx);
    setup_common_vars(req, &ctx, "");
    template_set(&ctx, "page_title", "ShopKart - Online Shopping Site for Mobiles, Electronics, Fashion & More");

    char *html = render_template("home.html", &ctx);
    if (html) {
        response_set_html(resp, html);
        free(html);
    } else {
        handle_404(req, resp);
    }
    template_context_free(&ctx);
}

/* ================= CATALOG / PRODUCTS HANDLER ================= */
static void handle_products(const HttpRequest *req, HttpResponse *resp) {
    ParamList params;
    parse_query_string(req->query_string, &params);

    const char *cat = param_get(&params, "cat");
    const char *sub = param_get(&params, "sub");
    const char *q = param_get(&params, "q");
    const char *min_price_str = param_get(&params, "min_price");
    const char *max_price_str = param_get(&params, "max_price");
    const char *min_rating_str = param_get(&params, "min_rating");
    const char *brand = param_get(&params, "brand");
    const char *sort_str = param_get(&params, "sort");
    const char *algo_str = param_get(&params, "algo");

    int min_price = (min_price_str && min_price_str[0] != '\0') ? atoi(min_price_str) : 0;
    int max_price = (max_price_str && max_price_str[0] != '\0') ? atoi(max_price_str) : 1000000;
    double min_rating = (min_rating_str && min_rating_str[0] != '\0') ? atof(min_rating_str) : 0.0;

    SortKey sort_key = SORT_KEY_RELEVANCE;
    if (sort_str) {
        if (strcmp(sort_str, "price_asc") == 0) sort_key = SORT_KEY_PRICE_ASC;
        else if (strcmp(sort_str, "price_desc") == 0) sort_key = SORT_KEY_PRICE_DESC;
        else if (strcmp(sort_str, "rating_desc") == 0) sort_key = SORT_KEY_RATING_DESC;
        else if (strcmp(sort_str, "discount_desc") == 0) sort_key = SORT_KEY_DISCOUNT_DESC;
    }

    SortAlgo sort_algo = ALGO_MERGE_SORT;
    if (algo_str && strcmp(algo_str, "quick") == 0) {
        sort_algo = ALGO_QUICK_SORT;
    }

    /* Step 1: Filter candidates from all products */
    const Product *all_prods = product_get_all();
    int total_all = product_count_total();

    Product *candidates = (Product *)malloc(sizeof(Product) * MAX_PRODUCTS);
    Product *price_filtered = (Product *)malloc(sizeof(Product) * MAX_PRODUCTS);
    Product *final_list = (Product *)malloc(sizeof(Product) * MAX_PRODUCTS);
    if (!candidates || !price_filtered || !final_list) {
        if (candidates) free(candidates);
        if (price_filtered) free(price_filtered);
        if (final_list) free(final_list);
        handle_404(req, resp);
        return;
    }

    int candidate_count = 0;

    if (q && q[0] != '\0') {
        candidate_count = linear_search_products(all_prods, total_all, q, candidates, MAX_PRODUCTS);
    } else {
        for (int i = 0; i < total_all && candidate_count < MAX_PRODUCTS; i++) {
            if (cat && cat[0] != '\0' && str_case_cmp(all_prods[i].category, cat) != 0) {
                continue;
            }
            if (sub && sub[0] != '\0' && str_case_cmp(all_prods[i].subcategory, sub) != 0) {
                continue;
            }
            candidates[candidate_count++] = all_prods[i];
        }
    }

    /* Step 2: Binary Search Price Range Filter */
    /* To apply binary search bounds, we sort candidates by price first */
    unsigned long long pre_sort_comps = 0;
    merge_sort_products(candidates, candidate_count, SORT_KEY_PRICE_ASC, &pre_sort_comps);

    int price_count = binary_search_filter_price_range(candidates, candidate_count, min_price, max_price, price_filtered, MAX_PRODUCTS);

    /* Step 3: Brand and Rating filtering */
    int final_count = 0;

    for (int i = 0; i < price_count && final_count < MAX_PRODUCTS; i++) {
        if (min_rating > 0.0 && price_filtered[i].rating < min_rating) {
            continue;
        }
        if (brand && brand[0] != '\0' && str_case_cmp(price_filtered[i].brand, brand) != 0) {
            continue;
        }
        final_list[final_count++] = price_filtered[i];
    }

    /* Step 4: Sort with user-selected DSA Engine */
    unsigned long long comps = 0;
    double t_start = get_time_ms();

    if (sort_algo == ALGO_QUICK_SORT) {
        quick_sort_products(final_list, final_count, sort_key, &comps);
    } else {
        merge_sort_products(final_list, final_count, sort_key, &comps);
    }
    double t_end = get_time_ms();
    double elapsed_ms = t_end - t_start;

    /* Build Dynamic HTML Cards */
    char *cards_buffer = (char *)malloc(final_count * 1024 + 512);
    if (!cards_buffer) {
        free(candidates);
        free(price_filtered);
        free(final_list);
        handle_404(req, resp);
        return;
    }
    cards_buffer[0] = '\0';

    if (final_count == 0) {
        snprintf(cards_buffer, final_count * 1024 + 512,
            "<div class=\"empty-state\">\n"
            "  <div class=\"empty-state-icon\">&#128269;</div>\n"
            "  <h3>No Products Found</h3>\n"
            "  <p>Try adjusting your search query, price range, or filters.</p>\n"
            "  <a href=\"/products\" class=\"btn btn-primary\">View All Products</a>\n"
            "</div>\n");
    } else {
        char card_html[1024];
        for (int i = 0; i < final_count; i++) {
            render_product_card_html(&final_list[i], card_html, sizeof(card_html));
            strncat(cards_buffer, card_html, (final_count * 1024 + 512) - strlen(cards_buffer) - 1);
        }
    }

    /* Build Brand dropdown options */
    char brand_options[2048] = {0};
    const char *known_brands[] = {"Sony", "boAt", "JBL", "Apple", "Dell", "HP", "Samsung", "Nike", "Puma", "Lego", "LG", "Philips", "Tata", "Lays", "Nivea", "Yonex", "Penguin"};
    int num_known = (int)(sizeof(known_brands) / sizeof(known_brands[0]));
    for (int b = 0; b < num_known; b++) {
        char opt[128];
        int sel = (brand && str_case_cmp(brand, known_brands[b]) == 0);
        snprintf(opt, sizeof(opt), "<option value=\"%s\" %s>%s</option>\n",
                 known_brands[b], sel ? "selected" : "", known_brands[b]);
        strncat(brand_options, opt, sizeof(brand_options) - strlen(brand_options) - 1);
    }

    TemplateContext ctx;
    template_context_init(&ctx);
    setup_common_vars(req, &ctx, q ? q : "");

    char count_str[32];
    snprintf(count_str, sizeof(count_str), "%d", final_count);
    template_set(&ctx, "result_count", count_str);

    char label_str[128];
    if (q && q[0] != '\0') {
        snprintf(label_str, sizeof(label_str), "\"%s\"", q);
    } else if (cat && cat[0] != '\0') {
        snprintf(label_str, sizeof(label_str), "Category: %s", cat);
    } else {
        snprintf(label_str, sizeof(label_str), "All Products");
    }
    template_set(&ctx, "search_label", label_str);

    template_set(&ctx, "breadcrumb_category", cat ? cat : "All Categories");
    template_set(&ctx, "current_cat_param", cat ? cat : "");
    template_set(&ctx, "current_sub_param", sub ? sub : "");
    template_set(&ctx, "current_q_param", q ? q : "");
    template_set(&ctx, "current_sort_param", sort_str ? sort_str : "relevance");
    template_set(&ctx, "current_algo_param", algo_str ? algo_str : "merge");

    template_set(&ctx, "filter_min_price", min_price > 0 ? min_price_str : "");
    template_set(&ctx, "filter_max_price", max_price < 1000000 ? max_price_str : "");
    template_set(&ctx, "filter_min_rating", min_rating_str ? min_rating_str : "");
    template_set(&ctx, "filter_brand", brand ? brand : "");

    template_set(&ctx, "rating_4_checked", (min_rating == 4.0) ? "checked" : "");
    template_set(&ctx, "rating_35_checked", (min_rating == 3.5) ? "checked" : "");
    template_set(&ctx, "rating_3_checked", (min_rating == 3.0) ? "checked" : "");
    template_set(&ctx, "rating_all_checked", (min_rating == 0.0) ? "checked" : "");

    template_set(&ctx, "sort_relevance_sel", (sort_key == SORT_KEY_RELEVANCE) ? "selected" : "");
    template_set(&ctx, "sort_price_asc_sel", (sort_key == SORT_KEY_PRICE_ASC) ? "selected" : "");
    template_set(&ctx, "sort_price_desc_sel", (sort_key == SORT_KEY_PRICE_DESC) ? "selected" : "");
    template_set(&ctx, "sort_rating_desc_sel", (sort_key == SORT_KEY_RATING_DESC) ? "selected" : "");
    template_set(&ctx, "sort_discount_desc_sel", (sort_key == SORT_KEY_DISCOUNT_DESC) ? "selected" : "");

    template_set(&ctx, "algo_merge_sel", (sort_algo == ALGO_MERGE_SORT) ? "selected" : "");
    template_set(&ctx, "algo_quick_sel", (sort_algo == ALGO_QUICK_SORT) ? "selected" : "");

    template_set(&ctx, "algo_display_name", (sort_algo == ALGO_QUICK_SORT) ? "Quick Sort (Randomized Pivot)" : "Merge Sort (Stable)");

    char comps_str[32];
    snprintf(comps_str, sizeof(comps_str), "%lu", (unsigned long)comps);
    template_set(&ctx, "algo_comparisons", comps_str);

    char time_str[32];
    snprintf(time_str, sizeof(time_str), "%.3f ms", elapsed_ms);
    template_set(&ctx, "algo_time_str", time_str);

    template_set(&ctx, "filter_method_info", "Binary Search Lower/Upper Bound (Price Index)");
    template_set(&ctx, "brand_options_html", brand_options);
    template_set(&ctx, "products_grid_html", cards_buffer);
    template_set(&ctx, "page_title", "ShopKart - Product Catalog");

    char *html = render_template("products.html", &ctx);
    if (html) {
        response_set_html(resp, html);
        free(html);
    } else {
        handle_404(req, resp);
    }

    free(candidates);
    free(price_filtered);
    free(final_list);
    free(cards_buffer);
    template_context_free(&ctx);
}

/* ================= PRODUCT DETAIL HANDLER ================= */
static void handle_product_detail(const HttpRequest *req, HttpResponse *resp) {
    ParamList params;
    parse_query_string(req->query_string, &params);
    const char *id_str = param_get(&params, "id");
    int pid = id_str ? atoi(id_str) : 0;

    const Product *p = product_get_by_id(pid);
    if (!p) {
        handle_404(req, resp);
        return;
    }

    /* Record into Recently Viewed circular queue */
    const User *u = auth_get_user_by_session(req->session_token);
    const char *email = u ? u->email : "guest@shopkart.local";
    recently_viewed_add(email, p->id);

    TemplateContext ctx;
    template_context_init(&ctx);
    setup_common_vars(req, &ctx, "");

    char pid_str[32];
    snprintf(pid_str, sizeof(pid_str), "%d", p->id);
    template_set(&ctx, "product_id", pid_str);
    template_set(&ctx, "product_name", p->name);
    template_set(&ctx, "product_brand", p->brand);
    template_set(&ctx, "product_category", p->category);
    template_set(&ctx, "product_category_slug", p->category);
    template_set(&ctx, "product_subcategory", p->subcategory);
    template_set(&ctx, "product_subcategory_slug", p->subcategory);

    char price_str[32], mrp_str[32], disc_str[32], rating_str[32], rev_str[32], rev_approx_str[32];
    snprintf(price_str, sizeof(price_str), "%d", p->price);
    snprintf(mrp_str, sizeof(mrp_str), "%d", p->mrp);
    int disc = (p->mrp > 0) ? (int)(((p->mrp - p->price) * 100) / p->mrp) : 0;
    snprintf(disc_str, sizeof(disc_str), "%d", disc);
    snprintf(rating_str, sizeof(rating_str), "%.1f", p->rating);
    snprintf(rev_str, sizeof(rev_str), "%d", p->review_count);
    snprintf(rev_approx_str, sizeof(rev_approx_str), "%d", p->review_count / 8 + 10);

    template_set(&ctx, "product_price", price_str);
    template_set(&ctx, "product_mrp", mrp_str);
    template_set(&ctx, "product_discount_pct", disc_str);
    template_set(&ctx, "product_rating", rating_str);
    template_set(&ctx, "product_reviews", rev_str);
    template_set(&ctx, "product_review_approx", rev_approx_str);

    /* Stock status */
    if (p->stock > 10) {
        template_set(&ctx, "stock_status_class", "stock-in");
        template_set(&ctx, "stock_status_text", "In Stock");
    } else {
        template_set(&ctx, "stock_status_class", "stock-low");
        char low_buf[64];
        snprintf(low_buf, sizeof(low_buf), "Hurry, Only %d left in stock!", p->stock);
        template_set(&ctx, "stock_status_text", low_buf);
    }

    /* Assign icon and class */
    const char *icon = "&#128717;";
    if (strcmp(p->category, "electronics") == 0) icon = "&#127911;";
    else if (strcmp(p->category, "mobiles") == 0) icon = "&#128241;";
    else if (strcmp(p->category, "fashion") == 0) icon = "&#128084;";
    else if (strcmp(p->category, "home") == 0) icon = "&#128715;";
    else if (strcmp(p->category, "appliances") == 0) icon = "&#129482;";
    else if (strcmp(p->category, "food") == 0) icon = "&#127871;";
    else if (strcmp(p->category, "beauty") == 0) icon = "&#10024;";
    else if (strcmp(p->category, "toys") == 0) icon = "&#129528;";
    else if (strcmp(p->category, "sports") == 0) icon = "&#127951;";
    else if (strcmp(p->category, "books") == 0) icon = "&#128214;";
    template_set(&ctx, "product_icon", icon);

    char img_cls[64];
    snprintf(img_cls, sizeof(img_cls), "bg-%c%c%c%c", p->category[0], p->category[1], p->category[2], p->category[3]);
    template_set(&ctx, "product_img_class", img_cls);

    /* Highlights HTML */
    char hl_html[1024] = {0};
    char hl_copy[MAX_HIGHLIGHTS_LEN];
    snprintf(hl_copy, sizeof(hl_copy), "%s", p->highlights);
    char *token = strtok(hl_copy, "|");
    while (token) {
        while (*token == ' ') token++;
        char li[256];
        snprintf(li, sizeof(li), "<li>%s</li>\n", token);
        strncat(hl_html, li, sizeof(hl_html) - strlen(hl_html) - 1);
        token = strtok(NULL, "|");
    }
    template_set(&ctx, "product_highlights_html", hl_html);

    /* Specifications HTML */
    char specs_html[2048] = {0};
    char specs_copy[MAX_SPECS_LEN];
    snprintf(specs_copy, sizeof(specs_copy), "%s", p->specs);
    token = strtok(specs_copy, "|");
    while (token) {
        while (*token == ' ') token++;
        char *colon = strchr(token, ':');
        if (colon) {
            *colon = '\0';
            char *val = colon + 1;
            while (*val == ' ') val++;
            char tr[256];
            snprintf(tr, sizeof(tr), "<tr><td class=\"specs-key\">%s</td><td class=\"specs-val\">%s</td></tr>\n", token, val);
            strncat(specs_html, tr, sizeof(specs_html) - strlen(specs_html) - 1);
        }
        token = strtok(NULL, "|");
    }
    template_set(&ctx, "product_specs_html", specs_html);

    /* Similar Products */
    char sim_html[4096] = {0};
    const Product *all_prods = product_get_all();
    int total_all = product_count_total();
    int sim_count = 0;
    for (int i = 0; i < total_all && sim_count < 6; i++) {
        if (all_prods[i].id != p->id && strcmp(all_prods[i].category, p->category) == 0) {
            char card[1024];
            render_product_card_html(&all_prods[i], card, sizeof(card));
            strncat(sim_html, card, sizeof(sim_html) - strlen(sim_html) - 1);
            sim_count++;
        }
    }
    template_set(&ctx, "similar_products_html", sim_html);

    char title_buf[300];
    snprintf(title_buf, sizeof(title_buf), "%s - Buy Online at Best Price on ShopKart", p->name);
    template_set(&ctx, "page_title", title_buf);

    char *html = render_template("product.html", &ctx);
    if (html) {
        response_set_html(resp, html);
        free(html);
    } else {
        handle_404(req, resp);
    }
    template_context_free(&ctx);
}

/* ================= AUTH ROUTE HANDLERS ================= */
static void handle_login_get(const HttpRequest *req, HttpResponse *resp, const char *error_msg) {
    TemplateContext ctx;
    template_context_init(&ctx);
    setup_common_vars(req, &ctx, "");
    template_set(&ctx, "page_title", "Login - ShopKart");
    template_set(&ctx, "login_email_value", "");

    if (error_msg && error_msg[0] != '\0') {
        char err_box[256];
        snprintf(err_box, sizeof(err_box), "<div class=\"auth-alert-error\">&#9888; %s</div>", error_msg);
        template_set(&ctx, "auth_error_html", err_box);
    } else {
        template_set(&ctx, "auth_error_html", "");
    }

    char *html = render_template("login.html", &ctx);
    if (html) {
        response_set_html(resp, html);
        free(html);
    } else {
        handle_404(req, resp);
    }
    template_context_free(&ctx);
}

static void handle_login_post(const HttpRequest *req, HttpResponse *resp) {
    ParamList params;
    parse_form_body(req->body, &params);
    const char *email = param_get(&params, "email");
    const char *pwd = param_get(&params, "password");

    char session_token[SESSION_TOKEN_LEN + 1];
    char err_buf[128];

    if (auth_login(email, pwd, session_token, err_buf)) {
        char cookie_hdr[256];
        snprintf(cookie_hdr, sizeof(cookie_hdr),
                 "Set-Cookie: shopkart_session=%s; Path=/; HttpOnly; SameSite=Lax\r\n",
                 session_token);
        response_add_header(resp, cookie_hdr);
        response_set_redirect(resp, "/");
    } else {
        handle_login_get(req, resp, err_buf);
    }
}

static void handle_signup_get(const HttpRequest *req, HttpResponse *resp, const char *error_msg) {
    TemplateContext ctx;
    template_context_init(&ctx);
    setup_common_vars(req, &ctx, "");
    template_set(&ctx, "page_title", "Sign Up - ShopKart");

    if (error_msg && error_msg[0] != '\0') {
        char err_box[256];
        snprintf(err_box, sizeof(err_box), "<div class=\"auth-alert-error\">&#9888; %s</div>", error_msg);
        template_set(&ctx, "auth_error_html", err_box);
    } else {
        template_set(&ctx, "auth_error_html", "");
    }

    char *html = render_template("signup.html", &ctx);
    if (html) {
        response_set_html(resp, html);
        free(html);
    } else {
        handle_404(req, resp);
    }
    template_context_free(&ctx);
}

static void handle_signup_post(const HttpRequest *req, HttpResponse *resp) {
    ParamList params;
    parse_form_body(req->body, &params);
    const char *name = param_get(&params, "name");
    const char *email = param_get(&params, "email");
    const char *pwd = param_get(&params, "password");

    char session_token[SESSION_TOKEN_LEN + 1];
    char err_buf[128];

    if (auth_signup(name, email, pwd, session_token, err_buf)) {
        char cookie_hdr[256];
        snprintf(cookie_hdr, sizeof(cookie_hdr),
                 "Set-Cookie: shopkart_session=%s; Path=/; HttpOnly; SameSite=Lax\r\n",
                 session_token);
        response_add_header(resp, cookie_hdr);
        response_set_redirect(resp, "/");
    } else {
        handle_signup_get(req, resp, err_buf);
    }
}

static void handle_logout(const HttpRequest *req, HttpResponse *resp) {
    auth_logout(req->session_token);
    response_add_header(resp, "Set-Cookie: shopkart_session=; Path=/; Expires=Thu, 01 Jan 1970 00:00:00 GMT\r\n");
    response_set_redirect(resp, "/");
}

/* ================= CART ROUTE HANDLERS ================= */
static void handle_cart_view(const HttpRequest *req, HttpResponse *resp) {
    const User *u = auth_get_user_by_session(req->session_token);
    const char *email = u ? u->email : "guest@shopkart.local";
    UserCart *cart = cart_get_for_user(email);

    TemplateContext ctx;
    template_context_init(&ctx);
    setup_common_vars(req, &ctx, "");
    template_set(&ctx, "page_title", "Shopping Cart - ShopKart");

    char count_str[32];
    snprintf(count_str, sizeof(count_str), "%d", cart ? cart->item_count : 0);
    template_set(&ctx, "cart_item_count", count_str);

    /* Check Undo Stack */
    if (cart && !stack_is_empty(&cart->undo_stack)) {
        CartUndoItem undo_item;
        stack_peek(&cart->undo_stack, &undo_item);
        char undo_banner[512];
        snprintf(undo_banner, sizeof(undo_banner),
            "<div class=\"undo-banner\">\n"
            "  <span>&#9851; Removed '%s' from cart.</span>\n"
            "  <form action=\"/cart/undo\" method=\"POST\">\n"
            "    <button type=\"submit\" class=\"undo-banner-btn\">UNDO</button>\n"
            "  </form>\n"
            "</div>", undo_item.product.name);
        template_set(&ctx, "undo_banner_html", undo_banner);
        template_set(&ctx, "undo_button_html",
            "<form action=\"/cart/undo\" method=\"POST\" style=\"display:inline;\">"
            "<button type=\"submit\" class=\"btn btn-secondary toolbar-btn\">&#8634; Undo Remove</button>"
            "</form>");
    } else {
        template_set(&ctx, "undo_banner_html", "");
        template_set(&ctx, "undo_button_html", "");
    }

    if (!cart || cart->item_count == 0) {
        template_set(&ctx, "cart_content_html",
            "<div class=\"empty-state\">\n"
            "  <div class=\"empty-state-icon\">&#128722;</div>\n"
            "  <h3>Your Cart is Empty!</h3>\n"
            "  <p>Explore our wide selection of products and discover great deals.</p>\n"
            "  <a href=\"/products\" class=\"btn btn-primary\">Shop Now</a>\n"
            "</div>");
        template_set(&ctx, "cart_price_sidebar_html", "");
    } else {
        /* Render Cart Items */
        char *items_buf = (char *)malloc(cart->item_count * 2048 + 512);
        if (!items_buf) {
            handle_404(req, resp);
            template_context_free(&ctx);
            return;
        }
        items_buf[0] = '\0';

        int total_mrp = 0;
        int total_price = 0;

        for (int i = 0; i < cart->item_count; i++) {
            const Product *p = product_get_by_id(cart->items[i].product_id);
            if (!p) continue;

            int item_mrp = p->mrp * cart->items[i].quantity;
            int item_price = p->price * cart->items[i].quantity;
            total_mrp += item_mrp;
            total_price += item_price;

            int disc_pct = (p->mrp > 0) ? (int)(((p->mrp - p->price) * 100) / p->mrp) : 0;

            char row_html[2048];
            snprintf(row_html, sizeof(row_html),
                "<div class=\"cart-item-row\">\n"
                "  <div class=\"cart-item-img bg-%c%c%c%c\">\n"
                "    <span>&#128717;</span>\n"
                "  </div>\n"
                "  <div class=\"cart-item-details\">\n"
                "    <h3 class=\"cart-item-title\"><a href=\"/product?id=%d\">%s</a></h3>\n"
                "    <div class=\"cart-item-seller\">Seller: Official %s Retail</div>\n"
                "    <div class=\"price-row\">\n"
                "      <span class=\"current-price\">&#8377;%d</span>\n"
                "      <span class=\"original-price\">&#8377;%d</span>\n"
                "      <span class=\"discount-tag\">%d%% Off</span>\n"
                "    </div>\n"
                "    <div class=\"cart-item-actions\">\n"
                "      <div class=\"qty-control-form\">\n"
                "        <form action=\"/cart/update\" method=\"POST\" style=\"display:inline;\">\n"
                "          <input type=\"hidden\" name=\"id\" value=\"%d\">\n"
                "          <input type=\"hidden\" name=\"quantity\" value=\"%d\">\n"
                "          <button type=\"submit\" class=\"qty-btn\">-</button>\n"
                "        </form>\n"
                "        <span class=\"qty-val\">%d</span>\n"
                "        <form action=\"/cart/update\" method=\"POST\" style=\"display:inline;\">\n"
                "          <input type=\"hidden\" name=\"id\" value=\"%d\">\n"
                "          <input type=\"hidden\" name=\"quantity\" value=\"%d\">\n"
                "          <button type=\"submit\" class=\"qty-btn\">+</button>\n"
                "        </form>\n"
                "      </div>\n"
                "      <form action=\"/cart/remove\" method=\"POST\" style=\"display:inline;\">\n"
                "        <input type=\"hidden\" name=\"id\" value=\"%d\">\n"
                "        <button type=\"submit\" class=\"cart-action-btn\">REMOVE</button>\n"
                "      </form>\n"
                "    </div>\n"
                "  </div>\n"
                "</div>\n",
                p->category[0], p->category[1], p->category[2], p->category[3],
                p->id, p->name,
                p->brand,
                item_price, item_mrp, disc_pct,
                p->id, cart->items[i].quantity - 1,
                cart->items[i].quantity,
                p->id, cart->items[i].quantity + 1,
                p->id);
            strncat(items_buf, row_html, (cart->item_count * 2048 + 512) - strlen(items_buf) - 1);
        }

        /* Place Order Bar */
        strncat(items_buf,
            "<div class=\"place-order-box\">\n"
            "  <a href=\"/checkout\" class=\"btn btn-primary btn-place-order\">PLACE ORDER</a>\n"
            "</div>\n", (cart->item_count * 2048 + 512) - strlen(items_buf) - 1);
        template_set(&ctx, "cart_content_html", items_buf);
        free(items_buf);

        /* Price Sidebar */
        int total_savings = total_mrp - total_price;
        char sidebar_html[2048];
        snprintf(sidebar_html, sizeof(sidebar_html),
            "<aside class=\"cart-price-sidebar\">\n"
            "  <div class=\"price-sidebar-header\">PRICE DETAILS</div>\n"
            "  <div class=\"price-breakdown-list\">\n"
            "    <div class=\"breakdown-row\"><span>Price (%d items)</span><span>&#8377;%d</span></div>\n"
            "    <div class=\"breakdown-row text-green\"><span>Discount</span><span>- &#8377;%d</span></div>\n"
            "    <div class=\"breakdown-row text-green\"><span>Delivery Charges</span><span>FREE</span></div>\n"
            "  </div>\n"
            "  <div class=\"total-amount-row\">\n"
            "    <span>Total Amount</span><span>&#8377;%d</span>\n"
            "  </div>\n"
            "  <div class=\"savings-banner\">\n"
            "    &#9989; You will save &#8377;%d on this order\n"
            "  </div>\n"
            "</aside>\n",
            cart->item_count, total_mrp, total_savings, total_price, total_savings);
        template_set(&ctx, "cart_price_sidebar_html", sidebar_html);
    }

    char *html = render_template("cart.html", &ctx);
    if (html) {
        response_set_html(resp, html);
        free(html);
    } else {
        handle_404(req, resp);
    }
    template_context_free(&ctx);
}

static void handle_cart_add_post(const HttpRequest *req, HttpResponse *resp) {
    const User *u = auth_get_user_by_session(req->session_token);
    const char *email = u ? u->email : "guest@shopkart.local";

    ParamList params;
    parse_form_body(req->body, &params);
    const char *id_str = param_get(&params, "id");
    const char *qty_str = param_get(&params, "quantity");

    int pid = id_str ? atoi(id_str) : 0;
    int qty = (qty_str && qty_str[0] != '\0') ? atoi(qty_str) : 1;
    if (qty <= 0) qty = 1;

    cart_add_item(email, pid, qty);
    response_set_redirect(resp, "/cart");
}

static void handle_cart_update_post(const HttpRequest *req, HttpResponse *resp) {
    const User *u = auth_get_user_by_session(req->session_token);
    const char *email = u ? u->email : "guest@shopkart.local";

    ParamList params;
    parse_form_body(req->body, &params);
    const char *id_str = param_get(&params, "id");
    const char *qty_str = param_get(&params, "quantity");

    int pid = id_str ? atoi(id_str) : 0;
    int qty = qty_str ? atoi(qty_str) : 1;

    cart_update_quantity(email, pid, qty);
    response_set_redirect(resp, "/cart");
}

static void handle_cart_remove_post(const HttpRequest *req, HttpResponse *resp) {
    const User *u = auth_get_user_by_session(req->session_token);
    const char *email = u ? u->email : "guest@shopkart.local";

    ParamList params;
    parse_form_body(req->body, &params);
    const char *id_str = param_get(&params, "id");
    int pid = id_str ? atoi(id_str) : 0;

    cart_remove_item(email, pid);
    response_set_redirect(resp, "/cart");
}

static void handle_cart_undo_post(const HttpRequest *req, HttpResponse *resp) {
    const User *u = auth_get_user_by_session(req->session_token);
    const char *email = u ? u->email : "guest@shopkart.local";

    cart_undo_remove(email);
    response_set_redirect(resp, "/cart");
}

/* ================= WISHLIST ROUTE HANDLERS ================= */
static void handle_wishlist_view(const HttpRequest *req, HttpResponse *resp) {
    const User *u = auth_get_user_by_session(req->session_token);
    if (!u) {
        response_set_redirect(resp, "/login");
        return;
    }

    UserCart *cart = cart_get_for_user(u->email);
    int wish_count = cart ? linkedlist_size(&cart->wishlist) : 0;

    TemplateContext ctx;
    template_context_init(&ctx);
    setup_common_vars(req, &ctx, "");
    template_set(&ctx, "page_title", "My Wishlist - ShopKart");

    char count_str[32];
    snprintf(count_str, sizeof(count_str), "%d", wish_count);
    template_set(&ctx, "wishlist_count", count_str);

    if (wish_count == 0) {
        template_set(&ctx, "wishlist_items_html",
            "<div class=\"empty-state\">\n"
            "  <div class=\"empty-state-icon\">&#9825;</div>\n"
            "  <h3>Empty Wishlist</h3>\n"
            "  <p>You have no items in your wishlist. Start adding items you love!</p>\n"
            "  <a href=\"/products\" class=\"btn btn-primary\">Discover Products</a>\n"
            "</div>");
    } else {
        int ids[MAX_PRODUCTS];
        int num = linkedlist_to_array(&cart->wishlist, ids, MAX_PRODUCTS);

        char *wish_buf = (char *)malloc(num * 2048 + 512);
        if (!wish_buf) {
            handle_404(req, resp);
            template_context_free(&ctx);
            return;
        }
        wish_buf[0] = '\0';

        for (int i = 0; i < num; i++) {
            const Product *p = product_get_by_id(ids[i]);
            if (!p) continue;

            int disc_pct = (p->mrp > 0) ? (int)(((p->mrp - p->price) * 100) / p->mrp) : 0;
            char row_html[2048];
            snprintf(row_html, sizeof(row_html),
                "<div class=\"wishlist-item-row\">\n"
                "  <div class=\"wishlist-img bg-%c%c%c%c\">\n"
                "    <span>&#128717;</span>\n"
                "  </div>\n"
                "  <div class=\"wishlist-details\">\n"
                "    <h3 class=\"cart-item-title\"><a href=\"/product?id=%d\">%s</a></h3>\n"
                "    <div class=\"rating-badge\">%.1f &#9733;</div>\n"
                "    <div class=\"price-row\">\n"
                "      <span class=\"current-price\">&#8377;%d</span>\n"
                "      <span class=\"original-price\">&#8377;%d</span>\n"
                "      <span class=\"discount-tag\">%d%% off</span>\n"
                "    </div>\n"
                "  </div>\n"
                "  <div class=\"wishlist-actions\">\n"
                "    <form action=\"/cart/add\" method=\"POST\" style=\"display:inline;\">\n"
                "      <input type=\"hidden\" name=\"id\" value=\"%d\">\n"
                "      <button type=\"submit\" class=\"btn btn-primary\">MOVE TO CART</button>\n"
                "    </form>\n"
                "    <form action=\"/wishlist/remove\" method=\"POST\" style=\"display:inline;\">\n"
                "      <input type=\"hidden\" name=\"id\" value=\"%d\">\n"
                "      <button type=\"submit\" class=\"btn btn-secondary\">REMOVE</button>\n"
                "    </form>\n"
                "  </div>\n"
                "</div>\n",
                p->category[0], p->category[1], p->category[2], p->category[3],
                p->id, p->name,
                p->rating,
                p->price, p->mrp, disc_pct,
                p->id,
                p->id);
            strncat(wish_buf, row_html, (num * 2048 + 512) - strlen(wish_buf) - 1);
        }
        template_set(&ctx, "wishlist_items_html", wish_buf);
        free(wish_buf);
    }

    char *html = render_template("wishlist.html", &ctx);
    if (html) {
        response_set_html(resp, html);
        free(html);
    } else {
        handle_404(req, resp);
    }
    template_context_free(&ctx);
}

static void handle_wishlist_add_post(const HttpRequest *req, HttpResponse *resp) {
    const User *u = auth_get_user_by_session(req->session_token);
    if (!u) {
        response_set_redirect(resp, "/login");
        return;
    }

    ParamList params;
    parse_form_body(req->body, &params);
    const char *id_str = param_get(&params, "id");
    int pid = id_str ? atoi(id_str) : 0;

    wishlist_add(u->email, pid);
    response_set_redirect(resp, "/wishlist");
}

static void handle_wishlist_remove_post(const HttpRequest *req, HttpResponse *resp) {
    const User *u = auth_get_user_by_session(req->session_token);
    if (!u) {
        response_set_redirect(resp, "/login");
        return;
    }

    ParamList params;
    parse_form_body(req->body, &params);
    const char *id_str = param_get(&params, "id");
    int pid = id_str ? atoi(id_str) : 0;

    wishlist_remove(u->email, pid);
    response_set_redirect(resp, "/wishlist");
}

/* ================= CHECKOUT ROUTE HANDLERS ================= */
static void handle_checkout_view(const HttpRequest *req, HttpResponse *resp, const char *err_msg) {
    const User *u = auth_get_user_by_session(req->session_token);
    if (!u) {
        response_set_redirect(resp, "/login");
        return;
    }

    UserCart *cart = cart_get_for_user(u->email);
    if (!cart || cart->item_count == 0) {
        response_set_redirect(resp, "/cart");
        return;
    }

    TemplateContext ctx;
    template_context_init(&ctx);
    setup_common_vars(req, &ctx, "");
    template_set(&ctx, "page_title", "Checkout - ShopKart");
    template_set(&ctx, "checkout_user_name", u->name);
    template_set(&ctx, "checkout_user_email", u->email);

    if (err_msg && err_msg[0] != '\0') {
        char err_box[256];
        snprintf(err_box, sizeof(err_box), "<div class=\"auth-alert-error\">&#9888; %s</div>", err_msg);
        template_set(&ctx, "checkout_error_html", err_box);
    } else {
        template_set(&ctx, "checkout_error_html", "");
    }

    /* Checkout summary sidebar */
    int total_mrp = 0;
    int total_price = 0;
    for (int i = 0; i < cart->item_count; i++) {
        const Product *p = product_get_by_id(cart->items[i].product_id);
        if (p) {
            total_mrp += p->mrp * cart->items[i].quantity;
            total_price += p->price * cart->items[i].quantity;
        }
    }
    int total_savings = total_mrp - total_price;

    char side_html[2048];
    snprintf(side_html, sizeof(side_html),
        "<aside class=\"cart-price-sidebar\">\n"
        "  <div class=\"price-sidebar-header\">ORDER SUMMARY (%d items)</div>\n"
        "  <div class=\"price-breakdown-list\">\n"
        "    <div class=\"breakdown-row\"><span>Total MRP</span><span>&#8377;%d</span></div>\n"
        "    <div class=\"breakdown-row text-green\"><span>Discount</span><span>- &#8377;%d</span></div>\n"
        "    <div class=\"breakdown-row text-green\"><span>Delivery</span><span>FREE</span></div>\n"
        "  </div>\n"
        "  <div class=\"total-amount-row\">\n"
        "    <span>Total Payable</span><span>&#8377;%d</span>\n"
        "  </div>\n"
        "  <div class=\"savings-banner\">\n"
        "    &#9989; You save &#8377;%d on this order\n"
        "  </div>\n"
        "</aside>\n",
        cart->item_count, total_mrp, total_savings, total_price, total_savings);
    template_set(&ctx, "checkout_sidebar_html", side_html);

    char *html = render_template("checkout.html", &ctx);
    if (html) {
        response_set_html(resp, html);
        free(html);
    } else {
        handle_404(req, resp);
    }
    template_context_free(&ctx);
}

static void handle_checkout_post(const HttpRequest *req, HttpResponse *resp) {
    const User *u = auth_get_user_by_session(req->session_token);
    if (!u) {
        response_set_redirect(resp, "/login");
        return;
    }

    ParamList params;
    parse_form_body(req->body, &params);
    const char *name = param_get(&params, "name");
    const char *phone = param_get(&params, "phone");
    const char *pincode = param_get(&params, "pincode");
    const char *city = param_get(&params, "city");
    const char *state = param_get(&params, "state");
    const char *address = param_get(&params, "address");
    const char *payment = param_get(&params, "payment_method");

    if (!name || !phone || !pincode || !city || !state || !address ||
        strlen(phone) != 10 || strlen(pincode) != 6) {
        handle_checkout_view(req, resp, "Please provide a valid 10-digit mobile number, 6-digit pincode, and complete address.");
        return;
    }

    char full_addr[512];
    snprintf(full_addr, sizeof(full_addr), "%s, %s, %s, %s - %s (Phone: %s)",
             name, address, city, state, pincode, phone);

    char order_id[32];
    if (order_create(u->email, full_addr, payment, order_id)) {
        char redirect_url[64];
        snprintf(redirect_url, sizeof(redirect_url), "/orders?new_order=%s", order_id);
        response_set_redirect(resp, redirect_url);
    } else {
        handle_checkout_view(req, resp, "Failed to process order. Please check your cart.");
    }
}

/* ================= ORDERS HISTORY HANDLER ================= */
static void handle_orders_view(const HttpRequest *req, HttpResponse *resp) {
    const User *u = auth_get_user_by_session(req->session_token);
    if (!u) {
        response_set_redirect(resp, "/login");
        return;
    }

    ParamList params;
    parse_query_string(req->query_string, &params);
    const char *new_order_id = param_get(&params, "new_order");

    TemplateContext ctx;
    template_context_init(&ctx);
    setup_common_vars(req, &ctx, "");
    template_set(&ctx, "page_title", "My Orders - ShopKart");

    if (new_order_id && new_order_id[0] != '\0') {
        char banner[512];
        snprintf(banner, sizeof(banner),
            "<div class=\"order-success-banner\">\n"
            "  <span>&#9989; Order Placed Successfully! Your Order ID is <strong>%s</strong>. Thank you for shopping with ShopKart!</span>\n"
            "</div>", new_order_id);
        template_set(&ctx, "order_success_banner_html", banner);
    } else {
        template_set(&ctx, "order_success_banner_html", "");
    }

    Order user_orders[MAX_ORDERS];
    int count = order_get_history(u->email, user_orders, MAX_ORDERS);

    if (count == 0) {
        template_set(&ctx, "orders_list_html",
            "<div class=\"empty-state\">\n"
            "  <div class=\"empty-state-icon\">&#128230;</div>\n"
            "  <h3>No Orders Yet</h3>\n"
            "  <p>You haven't placed any orders yet. Start exploring our vast catalog!</p>\n"
            "  <a href=\"/products\" class=\"btn btn-primary\">Start Shopping</a>\n"
            "</div>");
    } else {
        char *orders_buf = (char *)malloc(count * 2048 + 512);
        if (!orders_buf) {
            handle_404(req, resp);
            template_context_free(&ctx);
            return;
        }
        orders_buf[0] = '\0';

        for (int i = 0; i < count; i++) {
            const Product *p = product_get_by_id(user_orders[i].product_id);
            const char *pname = p ? p->name : "Product";
            char card_html[2048];
            snprintf(card_html, sizeof(card_html),
                "<div class=\"order-card\">\n"
                "  <div class=\"order-card-header\">\n"
                "    <span>Order ID: <span class=\"order-id-badge\">%s</span></span>\n"
                "    <span>Order Date: <strong>%s</strong></span>\n"
                "    <span>Payment: <strong>%s</strong></span>\n"
                "  </div>\n"
                "  <div class=\"order-card-body\">\n"
                "    <div class=\"order-prod-img\">&#128230;</div>\n"
                "    <div class=\"order-prod-info\">\n"
                "      <div class=\"order-prod-name\"><a href=\"/product?id=%d\">%s</a></div>\n"
                "      <div class=\"order-prod-meta\">Qty: %d | Total Price: &#8377;%d</div>\n"
                "      <div class=\"order-prod-meta\">Delivering to: %s</div>\n"
                "    </div>\n"
                "    <div class=\"order-status-badge\">&#9679; Order Confirmed</div>\n"
                "  </div>\n"
                "</div>\n",
                user_orders[i].order_id,
                user_orders[i].order_date,
                user_orders[i].payment_method,
                user_orders[i].product_id,
                pname,
                user_orders[i].quantity,
                user_orders[i].total_price,
                user_orders[i].delivery_address);
            strncat(orders_buf, card_html, (count * 2048 + 512) - strlen(orders_buf) - 1);
        }
        template_set(&ctx, "orders_list_html", orders_buf);
        free(orders_buf);
    }

    char *html = render_template("orders.html", &ctx);
    if (html) {
        response_set_html(resp, html);
        free(html);
    } else {
        handle_404(req, resp);
    }
    template_context_free(&ctx);
}

/* Comparator for baseline qsort */
static int qsort_baseline_comparator(const void *a, const void *b) {
    const Product *pa = (const Product *)a;
    const Product *pb = (const Product *)b;
    if (pa->price != pb->price) {
        return pa->price - pb->price;
    }
    return pa->id - pb->id;
}

/* ================= DSA BENCHMARK HANDLER ================= */
static void handle_dsa_view(const HttpRequest *req, HttpResponse *resp) {
    ParamList params;
    parse_query_string(req->query_string, &params);
    const char *run_flag = param_get(&params, "run");
    const char *n_str = param_get(&params, "n");

    int n = 1000;
    if (n_str) {
        int parsed_n = atoi(n_str);
        if (parsed_n == 100 || parsed_n == 1000 || parsed_n == 10000) {
            n = parsed_n;
        }
    }

    TemplateContext ctx;
    template_context_init(&ctx);
    setup_common_vars(req, &ctx, "");
    template_set(&ctx, "page_title", "DSA Engine & Live Benchmark - ShopKart");

    template_set(&ctx, "bench_n_100_sel", (n == 100) ? "selected" : "");
    template_set(&ctx, "bench_n_1000_sel", (n == 1000) ? "selected" : "");
    template_set(&ctx, "bench_n_10000_sel", (n == 10000) ? "selected" : "");

    if (run_flag && strcmp(run_flag, "1") == 0) {
        /* Generate N pseudo-random products in memory */
        Product *arr_merge = (Product *)malloc(sizeof(Product) * (size_t)n);
        Product *arr_quick = (Product *)malloc(sizeof(Product) * (size_t)n);
        Product *arr_baseline = (Product *)malloc(sizeof(Product) * (size_t)n);

        if (arr_merge && arr_quick && arr_baseline) {
            srand(12345); /* Consistent seed for benchmark comparisons */
            for (int i = 0; i < n; i++) {
                arr_merge[i].id = i + 1;
                arr_merge[i].price = rand() % 50000 + 100;
                arr_merge[i].mrp = arr_merge[i].price + 500;
                arr_merge[i].rating = 3.0 + (rand() % 20) / 10.0;
                snprintf(arr_merge[i].name, sizeof(arr_merge[i].name), "Product %d", i + 1);

                arr_quick[i] = arr_merge[i];
                arr_baseline[i] = arr_merge[i];
            }

            /* 1. Merge Sort */
            unsigned long long comps_merge = 0;
            double t0 = get_time_ms();
            merge_sort_products(arr_merge, n, SORT_KEY_PRICE_ASC, &comps_merge);
            double t1 = get_time_ms();
            double time_merge = t1 - t0;

            /* 2. Quick Sort */
            unsigned long long comps_quick = 0;
            double t2 = get_time_ms();
            quick_sort_products(arr_quick, n, SORT_KEY_PRICE_ASC, &comps_quick);
            double t3 = get_time_ms();
            double time_quick = t3 - t2;

            /* 3. Baseline qsort */
            double t4 = get_time_ms();
            qsort(arr_baseline, (size_t)n, sizeof(Product), qsort_baseline_comparator);
            double t5 = get_time_ms();
            double time_baseline = t5 - t4;

            char results_html[4096];
            snprintf(results_html, sizeof(results_html),
                "<div class=\"bench-results-box\">\n"
                "  <h3>&#128202; Live Benchmark Results for N = %d Elements</h3>\n"
                "  <table class=\"dsa-table\">\n"
                "    <thead>\n"
                "      <tr>\n"
                "        <th>Algorithm</th>\n"
                "        <th>Elements (N)</th>\n"
                "        <th>Comparisons</th>\n"
                "        <th>Execution Time</th>\n"
                "        <th>Stable?</th>\n"
                "        <th>Time Complexity</th>\n"
                "      </tr>\n"
                "    </thead>\n"
                "    <tbody>\n"
                "      <tr>\n"
                "        <td><strong>Merge Sort</strong></td>\n"
                "        <td>%d</td>\n"
                "        <td>%lu</td>\n"
                "        <td><strong>%.4f ms</strong></td>\n"
                "        <td><span class=\"badge-yes\">Yes</span></td>\n"
                "        <td>O(n log n) guaranteed</td>\n"
                "      </tr>\n"
                "      <tr>\n"
                "        <td><strong>Quick Sort (Randomized Pivot)</strong></td>\n"
                "        <td>%d</td>\n"
                "        <td>%lu</td>\n"
                "        <td><strong>%.4f ms</strong></td>\n"
                "        <td><span class=\"badge-no\">No</span></td>\n"
                "        <td>O(n log n) avg, O(n&sup2;) worst</td>\n"
                "      </tr>\n"
                "      <tr>\n"
                "        <td><strong>Standard C qsort (Baseline)</strong></td>\n"
                "        <td>%d</td>\n"
                "        <td>N/A (C library)</td>\n"
                "        <td><strong>%.4f ms</strong></td>\n"
                "        <td><span class=\"badge-no\">No</span></td>\n"
                "        <td>Compiler C runtime</td>\n"
                "      </tr>\n"
                "    </tbody>\n"
                "  </table>\n"
                "</div>",
                n,
                n, (unsigned long)comps_merge, time_merge,
                n, (unsigned long)comps_quick, time_quick,
                n, time_baseline);
            template_set(&ctx, "benchmark_results_html", results_html);

            free(arr_merge);
            free(arr_quick);
            free(arr_baseline);
        } else {
            template_set(&ctx, "benchmark_results_html", "<p>Error allocating benchmark memory.</p>");
        }
    } else {
        template_set(&ctx, "benchmark_results_html", "");
    }

    char *html = render_template("dsa.html", &ctx);
    if (html) {
        response_set_html(resp, html);
        free(html);
    } else {
        handle_404(req, resp);
    }
    template_context_free(&ctx);
}

/* ================= STATIC FILE HANDLER ================= */
static void handle_static(const HttpRequest *req, HttpResponse *resp) {
    if (strstr(req->path, "..")) {
        response_set_status(resp, 403, "Forbidden");
        response_set_html(resp, "<h1>403 Forbidden</h1>");
        return;
    }

    const char *rel_path = req->path;
    if (rel_path[0] == '/') {
        rel_path++;
    }

    size_t file_size = 0;
    char *content = load_file(rel_path, &file_size);
    if (!content) {
        handle_404(req, resp);
        return;
    }

    const char *ext = strrchr(rel_path, '.');
    if (ext) {
        if (str_case_cmp(ext, ".css") == 0) {
            snprintf(resp->content_type, sizeof(resp->content_type), "text/css");
        } else if (str_case_cmp(ext, ".svg") == 0) {
            snprintf(resp->content_type, sizeof(resp->content_type), "image/svg+xml");
        } else if (str_case_cmp(ext, ".png") == 0) {
            snprintf(resp->content_type, sizeof(resp->content_type), "image/png");
        } else if (str_case_cmp(ext, ".jpg") == 0 || str_case_cmp(ext, ".jpeg") == 0) {
            snprintf(resp->content_type, sizeof(resp->content_type), "image/jpeg");
        } else {
            snprintf(resp->content_type, sizeof(resp->content_type), "application/octet-stream");
        }
    } else {
        snprintf(resp->content_type, sizeof(resp->content_type), "application/octet-stream");
    }

    response_set_status(resp, 200, "OK");
    response_set_body(resp, content, file_size);
    free(content);
}

/* ================= ROUTER DISPATCHER ================= */
static void route_request(const HttpRequest *req, HttpResponse *resp) {
    /* GET Routes */
    if (strcmp(req->method, "GET") == 0) {
        if (strcmp(req->path, "/") == 0) {
            handle_home(req, resp);
            return;
        }
        if (strcmp(req->path, "/products") == 0) {
            handle_products(req, resp);
            return;
        }
        if (strcmp(req->path, "/product") == 0) {
            handle_product_detail(req, resp);
            return;
        }
        if (strcmp(req->path, "/search") == 0) {
            /* Redirect search queries to products catalog */
            char redirect_url[MAX_PATH_LEN + 32];
            snprintf(redirect_url, sizeof(redirect_url), "/products?q=%s", req->query_string);
            response_set_redirect(resp, redirect_url);
            return;
        }
        if (strcmp(req->path, "/login") == 0) {
            handle_login_get(req, resp, NULL);
            return;
        }
        if (strcmp(req->path, "/signup") == 0) {
            handle_signup_get(req, resp, NULL);
            return;
        }
        if (strcmp(req->path, "/logout") == 0) {
            handle_logout(req, resp);
            return;
        }
        if (strcmp(req->path, "/cart") == 0) {
            handle_cart_view(req, resp);
            return;
        }
        if (strcmp(req->path, "/wishlist") == 0) {
            handle_wishlist_view(req, resp);
            return;
        }
        if (strcmp(req->path, "/checkout") == 0) {
            handle_checkout_view(req, resp, NULL);
            return;
        }
        if (strcmp(req->path, "/orders") == 0) {
            handle_orders_view(req, resp);
            return;
        }
        if (strcmp(req->path, "/dsa") == 0) {
            handle_dsa_view(req, resp);
            return;
        }
        if (strncmp(req->path, "/static/", 8) == 0) {
            handle_static(req, resp);
            return;
        }
        if (strcmp(req->path, "/favicon.ico") == 0) {
            response_set_status(resp, 404, "Not Found");
            response_set_body(resp, "", 0);
            return;
        }
    }

    /* POST Routes */
    if (strcmp(req->method, "POST") == 0) {
        if (strcmp(req->path, "/login") == 0) {
            handle_login_post(req, resp);
            return;
        }
        if (strcmp(req->path, "/signup") == 0) {
            handle_signup_post(req, resp);
            return;
        }
        if (strcmp(req->path, "/cart/add") == 0) {
            handle_cart_add_post(req, resp);
            return;
        }
        if (strcmp(req->path, "/cart/update") == 0) {
            handle_cart_update_post(req, resp);
            return;
        }
        if (strcmp(req->path, "/cart/remove") == 0) {
            handle_cart_remove_post(req, resp);
            return;
        }
        if (strcmp(req->path, "/cart/undo") == 0) {
            handle_cart_undo_post(req, resp);
            return;
        }
        if (strcmp(req->path, "/wishlist/add") == 0) {
            handle_wishlist_add_post(req, resp);
            return;
        }
        if (strcmp(req->path, "/wishlist/remove") == 0) {
            handle_wishlist_remove_post(req, resp);
            return;
        }
        if (strcmp(req->path, "/checkout") == 0) {
            handle_checkout_post(req, resp);
            return;
        }
    }

    handle_404(req, resp);
}

/* Parse raw request buffer into HttpRequest structure */
static int parse_http_request(const char *buf, size_t buf_len, HttpRequest *req) {
    if (!buf || buf_len == 0 || !req) return 0;
    memset(req, 0, sizeof(HttpRequest));

    /* Find end of header section */
    const char *header_end = strstr(buf, "\r\n\r\n");
    const char *body_start = NULL;

    if (header_end) {
        body_start = header_end + 4;
    } else {
        header_end = strstr(buf, "\n\n");
        if (header_end) {
            body_start = header_end + 2;
        } else {
            return 0;
        }
    }

    /* Parse request line: METHOD PATH HTTP/1.x */
    const char *line_end = strstr(buf, "\r\n");
    if (!line_end) line_end = strstr(buf, "\n");
    if (!line_end) return 0;

    char req_line[MAX_PATH_LEN + 64];
    size_t req_line_len = (size_t)(line_end - buf);
    if (req_line_len >= sizeof(req_line)) {
        req_line_len = sizeof(req_line) - 1;
    }
    memcpy(req_line, buf, req_line_len);
    req_line[req_line_len] = '\0';

    char full_uri[MAX_PATH_LEN] = {0};
    char version[32] = {0};
    int matched = sscanf(req_line, "%15s %2047s %31s", req->method, full_uri, version);
    if (matched < 2) {
        return 0;
    }

    /* Split path and query string */
    char *qmark = strchr(full_uri, '?');
    if (qmark) {
        *qmark = '\0';
        snprintf(req->path, sizeof(req->path), "%s", full_uri);
        snprintf(req->query_string, sizeof(req->query_string), "%s", qmark + 1);
    } else {
        snprintf(req->path, sizeof(req->path), "%s", full_uri);
        req->query_string[0] = '\0';
    }

    /* Parse headers */
    const char *cur = line_end;
    if (*cur == '\r') cur += 2;
    else if (*cur == '\n') cur += 1;

    while (cur < header_end && req->header_count < MAX_HEADERS) {
        const char *next_line = strstr(cur, "\r\n");
        size_t next_line_step = 2;
        if (!next_line) {
            next_line = strstr(cur, "\n");
            next_line_step = 1;
        }
        if (!next_line || next_line > header_end) {
            next_line = header_end;
            next_line_step = 0;
        }

        size_t hlen = (size_t)(next_line - cur);
        if (hlen > 0) {
            char hbuf[1536];
            if (hlen >= sizeof(hbuf)) hlen = sizeof(hbuf) - 1;
            memcpy(hbuf, cur, hlen);
            hbuf[hlen] = '\0';

            char *colon = strchr(hbuf, ':');
            if (colon) {
                *colon = '\0';
                char *val = colon + 1;
                while (*val == ' ' || *val == '\t') val++;

                snprintf(req->headers[req->header_count].name,
                         sizeof(req->headers[req->header_count].name),
                         "%s", hbuf);
                snprintf(req->headers[req->header_count].value,
                         sizeof(req->headers[req->header_count].value),
                         "%s", val);
                req->header_count++;
            }
        }

        if (next_line_step == 0) break;
        cur = next_line + next_line_step;
    }

    /* Extract Session Token from Cookie */
    extract_session_token(req);

    /* Parse Body if Content-Length specified */
    const char *cl_val = request_get_header(req, "Content-Length");
    if (cl_val && body_start) {
        int content_len = atoi(cl_val);
        if (content_len > 0) {
            if (content_len > MAX_BODY_SIZE) {
                content_len = MAX_BODY_SIZE;
            }
            size_t available = buf_len - (size_t)(body_start - buf);
            size_t to_copy = ((size_t)content_len < available) ? (size_t)content_len : available;
            memcpy(req->body, body_start, to_copy);
            req->body[to_copy] = '\0';
            req->body_length = (int)to_copy;
        }
    }

    return 1;
}

static void send_full_response(socket_t client_sock, const HttpResponse *resp) {
    char header_buf[1024];
    int header_len = snprintf(header_buf, sizeof(header_buf),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %lu\r\n"
        "Connection: close\r\n"
        "%s"
        "\r\n",
        resp->status_code,
        resp->status_text,
        resp->content_type,
        (unsigned long)resp->body_length,
        resp->extra_headers);

    if (header_len > 0) {
        int total_sent = 0;
        while (total_sent < header_len) {
            int sent = send(client_sock, header_buf + total_sent, header_len - total_sent, 0);
            if (sent <= 0) break;
            total_sent += sent;
        }
    }

    if (resp->body && resp->body_length > 0) {
        size_t total_sent = 0;
        while (total_sent < resp->body_length) {
            int to_send = (int)(resp->body_length - total_sent);
            int sent = send(client_sock, resp->body + total_sent, to_send, 0);
            if (sent <= 0) break;
            total_sent += (size_t)sent;
        }
    }
}

int server_init(int port) {
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        fprintf(stderr, "WSAStartup failed\n");
        return -1;
    }
#endif

    g_server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (g_server_fd == INVALID_SOCK) {
        perror("socket creation failed");
        return -1;
    }

    int opt = 1;
#ifdef _WIN32
    setsockopt(g_server_fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt));
#else
    setsockopt(g_server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons((unsigned short)port);

    if (bind(g_server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        CLOSESOCKET(g_server_fd);
        g_server_fd = INVALID_SOCK;
        return -1;
    }

    if (listen(g_server_fd, 10) < 0) {
        perror("listen failed");
        CLOSESOCKET(g_server_fd);
        g_server_fd = INVALID_SOCK;
        return -1;
    }

    printf("[SERVER] ShopKart server running on http://localhost:%d\n", port);
    return 0;
}

void server_run(void) {
    if (g_server_fd == INVALID_SOCK) return;

    char *req_buf = (char *)malloc(MAX_HEADER_SIZE + MAX_BODY_SIZE + 1);
    if (!req_buf) {
        fprintf(stderr, "Failed to allocate request buffer\n");
        return;
    }

    while (1) {
        struct sockaddr_in client_addr;
        int client_len = sizeof(client_addr);
        socket_t client_sock = accept(g_server_fd, (struct sockaddr *)&client_addr,
#ifdef _WIN32
                                      &client_len
#else
                                      (socklen_t *)&client_len
#endif
        );

        if (client_sock == INVALID_SOCK) {
            break;
        }

        /* Read request */
        int total_read = 0;
        int max_to_read = MAX_HEADER_SIZE + MAX_BODY_SIZE;

        while (total_read < max_to_read) {
            int bytes = recv(client_sock, req_buf + total_read, max_to_read - total_read, 0);
            if (bytes <= 0) break;
            total_read += bytes;
            req_buf[total_read] = '\0';

            /* If header section complete */
            char *hdr_end = strstr(req_buf, "\r\n\r\n");
            if (hdr_end) {
                const char *cl = strstr(req_buf, "Content-Length:");
                if (!cl) cl = strstr(req_buf, "content-length:");
                if (cl) {
                    int body_needed = atoi(cl + 15);
                    size_t body_have = total_read - (size_t)(hdr_end + 4 - req_buf);
                    if ((int)body_have >= body_needed) {
                        break;
                    }
                } else {
                    break;
                }
            }
        }

        if (total_read > 0) {
            HttpRequest req;
            HttpResponse resp;
            response_init(&resp);

            if (parse_http_request(req_buf, (size_t)total_read, &req)) {
                route_request(&req, &resp);
                send_full_response(client_sock, &resp);
            }

            response_free(&resp);
        }

        CLOSESOCKET(client_sock);
    }

    free(req_buf);
}

void server_cleanup(void) {
    if (g_server_fd != INVALID_SOCK) {
        CLOSESOCKET(g_server_fd);
        g_server_fd = INVALID_SOCK;
    }
#ifdef _WIN32
    WSACleanup();
#endif
}
