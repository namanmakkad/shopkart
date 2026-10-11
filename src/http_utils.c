#include "http_utils.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

/* Convert a hexadecimal character to its integer value */
static int hex_val(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

/* Decode URL-encoded string (%XX and +) into dst buffer safely */
void url_decode(const char *src, char *dst, size_t dst_size) {
    if (!dst || dst_size == 0) {
        return;
    }
    if (!src) {
        dst[0] = '\0';
        return;
    }

    size_t out_idx = 0;
    size_t in_idx = 0;

    while (src[in_idx] != '\0' && out_idx + 1 < dst_size) {
        if (src[in_idx] == '+') {
            dst[out_idx++] = ' ';
            in_idx++;
        } else if (src[in_idx] == '%' && src[in_idx + 1] != '\0' && src[in_idx + 2] != '\0') {
            int h1 = hex_val(src[in_idx + 1]);
            int h2 = hex_val(src[in_idx + 2]);
            if (h1 >= 0 && h2 >= 0) {
                dst[out_idx++] = (char)((h1 << 4) | h2);
                in_idx += 3;
            } else {
                dst[out_idx++] = src[in_idx++];
            }
        } else {
            dst[out_idx++] = src[in_idx++];
        }
    }
    dst[out_idx] = '\0';
}

/* Parse a query string or URL-encoded form data into key-value pairs */
void parse_query_string(const char *qs, ParamList *params) {
    if (!params) {
        return;
    }
    params->count = 0;
    if (!qs || *qs == '\0') {
        return;
    }

    const char *p = qs;
    while (*p != '\0' && params->count < MAX_PARAMS) {
        /* Find end of current key-value pair */
        const char *amp = strchr(p, '&');
        size_t pair_len = amp ? (size_t)(amp - p) : strlen(p);

        if (pair_len > 0) {
            char raw_pair[2560];
            size_t copy_len = pair_len < sizeof(raw_pair) - 1 ? pair_len : sizeof(raw_pair) - 1;
            memcpy(raw_pair, p, copy_len);
            raw_pair[copy_len] = '\0';

            char *eq = strchr(raw_pair, '=');
            if (eq) {
                *eq = '\0';
                char *raw_val = eq + 1;
                url_decode(raw_pair, params->items[params->count].key, sizeof(params->items[params->count].key));
                url_decode(raw_val, params->items[params->count].value, sizeof(params->items[params->count].value));
            } else {
                url_decode(raw_pair, params->items[params->count].key, sizeof(params->items[params->count].key));
                params->items[params->count].value[0] = '\0';
            }
            params->count++;
        }

        if (!amp) {
            break;
        }
        p = amp + 1;
    }
}

/* Parse application/x-www-form-urlencoded body */
void parse_form_body(const char *body, ParamList *params) {
    parse_query_string(body, params);
}

/* Search for a parameter value by key in a ParamList */
const char *param_get(const ParamList *params, const char *key) {
    if (!params || !key) {
        return NULL;
    }
    for (int i = 0; i < params->count; i++) {
        if (strcmp(params->items[i].key, key) == 0) {
            return params->items[i].value;
        }
    }
    return NULL;
}

/* Escape HTML special characters into dst buffer with size check */
void html_escape(const char *src, char *dst, size_t dst_size) {
    if (!dst || dst_size == 0) {
        return;
    }
    if (!src) {
        dst[0] = '\0';
        return;
    }

    size_t out_idx = 0;
    for (size_t i = 0; src[i] != '\0' && out_idx + 1 < dst_size; i++) {
        const char *replacement = NULL;
        switch (src[i]) {
            case '&':  replacement = "&amp;"; break;
            case '<':  replacement = "&lt;"; break;
            case '>':  replacement = "&gt;"; break;
            case '"':  replacement = "&quot;"; break;
            case '\'': replacement = "&#39;"; break;
            default:   break;
        }

        if (replacement) {
            size_t rep_len = strlen(replacement);
            if (out_idx + rep_len < dst_size) {
                memcpy(&dst[out_idx], replacement, rep_len);
                out_idx += rep_len;
            } else {
                break;
            }
        } else {
            dst[out_idx++] = src[i];
        }
    }
    dst[out_idx] = '\0';
}
