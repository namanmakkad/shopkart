#include "render.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_INCLUDE_DEPTH 10

/* Dynamic string buffer to safely build output strings */
typedef struct {
    char *data;
    size_t len;
    size_t cap;
} DynamicBuffer;

/* Initialize a dynamic string buffer with default capacity */
static int buf_init(DynamicBuffer *buf, size_t initial_cap) {
    if (!buf) return 0;
    if (initial_cap < 64) initial_cap = 64;
    buf->data = (char *)malloc(initial_cap);
    if (!buf->data) {
        buf->len = 0;
        buf->cap = 0;
        return 0;
    }
    buf->data[0] = '\0';
    buf->len = 0;
    buf->cap = initial_cap;
    return 1;
}

/* Free resources allocated by dynamic string buffer */
static void buf_free(DynamicBuffer *buf) {
    if (buf && buf->data) {
        free(buf->data);
        buf->data = NULL;
        buf->len = 0;
        buf->cap = 0;
    }
}

/* Ensure dynamic string buffer has enough space for additional bytes */
static int buf_grow(DynamicBuffer *buf, size_t extra_bytes) {
    if (!buf || !buf->data) return 0;
    size_t required = buf->len + extra_bytes + 1;
    if (required <= buf->cap) return 1;

    size_t new_cap = buf->cap * 2;
    if (new_cap < required) {
        new_cap = required + 128;
    }
    char *new_data = (char *)realloc(buf->data, new_cap);
    if (!new_data) {
        return 0;
    }
    buf->data = new_data;
    buf->cap = new_cap;
    return 1;
}

/* Append a byte buffer of specified length to dynamic string buffer */
static int buf_append_len(DynamicBuffer *buf, const char *src, size_t len) {
    if (!buf || !src || len == 0) return 1;
    if (!buf_grow(buf, len)) return 0;
    memcpy(buf->data + buf->len, src, len);
    buf->len += len;
    buf->data[buf->len] = '\0';
    return 1;
}

/* Append a null-terminated string to dynamic string buffer */
static int buf_append_str(DynamicBuffer *buf, const char *str) {
    if (!str) return 1;
    return buf_append_len(buf, str, strlen(str));
}

/* Duplicate a string using safe bounded allocation */
static char *safe_strdup(const char *src) {
    if (!src) return NULL;
    size_t len = strlen(src);
    char *copy = (char *)malloc(len + 1);
    if (!copy) return NULL;
    memcpy(copy, src, len);
    copy[len] = '\0';
    return copy;
}

/* Initialize template context struct to zeros */
void template_context_init(TemplateContext *ctx) {
    if (!ctx) return;
    memset(ctx, 0, sizeof(TemplateContext));
}

/* Set or replace a variable in the template context */
void template_set(TemplateContext *ctx, const char *key, const char *value) {
    if (!ctx || !key) return;

    /* Check if key already exists */
    for (int i = 0; i < ctx->count; i++) {
        if (strcmp(ctx->vars[i].key, key) == 0) {
            free(ctx->vars[i].value);
            ctx->vars[i].value = safe_strdup(value ? value : "");
            return;
        }
    }

    /* Add new variable if capacity permits */
    if (ctx->count < MAX_TEMPLATE_VARS) {
        snprintf(ctx->vars[ctx->count].key, sizeof(ctx->vars[ctx->count].key), "%s", key);
        ctx->vars[ctx->count].value = safe_strdup(value ? value : "");
        ctx->count++;
    }
}

/* Look up a variable value by key in the template context */
const char *template_get(const TemplateContext *ctx, const char *key) {
    if (!ctx || !key) return "";
    for (int i = 0; i < ctx->count; i++) {
        if (strcmp(ctx->vars[i].key, key) == 0) {
            return ctx->vars[i].value ? ctx->vars[i].value : "";
        }
    }
    return "";
}

/* Free all heap-allocated values in the template context */
void template_context_free(TemplateContext *ctx) {
    if (!ctx) return;
    for (int i = 0; i < ctx->count; i++) {
        if (ctx->vars[i].value) {
            free(ctx->vars[i].value);
            ctx->vars[i].value = NULL;
        }
    }
    ctx->count = 0;
}

/* Load a raw file from disk in binary mode, returning heap-allocated buffer */
char *load_file(const char *filepath, size_t *out_size) {
    if (!filepath) return NULL;

    FILE *fp = fopen(filepath, "rb");
    if (!fp) return NULL;

    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return NULL;
    }

    long file_size = ftell(fp);
    if (file_size < 0) {
        fclose(fp);
        return NULL;
    }

    if (fseek(fp, 0, SEEK_SET) != 0) {
        fclose(fp);
        return NULL;
    }

    char *buffer = (char *)malloc((size_t)file_size + 1);
    if (!buffer) {
        fclose(fp);
        return NULL;
    }

    size_t read_bytes = fread(buffer, 1, (size_t)file_size, fp);
    fclose(fp);

    buffer[read_bytes] = '\0';
    if (out_size) {
        *out_size = read_bytes;
    }
    return buffer;
}

/* Recursive helper to render a template with inclusion depth limit */
static char *render_template_internal(const char *filename, TemplateContext *ctx, int depth) {
    if (!filename || depth > MAX_INCLUDE_DEPTH) {
        return NULL;
    }

    char filepath[MAX_VAR_KEY_LEN + 32];
    snprintf(filepath, sizeof(filepath), "templates/%s", filename);

    char *raw = load_file(filepath, NULL);
    if (!raw) {
        return NULL;
    }

    /* Pass 1: Resolve {{include:filename}} directives */
    DynamicBuffer pass1;
    if (!buf_init(&pass1, strlen(raw) + 128)) {
        free(raw);
        return NULL;
    }

    const char *p = raw;
    while (*p != '\0') {
        const char *inc_start = strstr(p, "{{include:");
        if (!inc_start) {
            buf_append_str(&pass1, p);
            break;
        }

        /* Append text prior to include tag */
        buf_append_len(&pass1, p, (size_t)(inc_start - p));

        const char *tag_val = inc_start + 10; /* Length of "{{include:" */
        const char *tag_end = strstr(tag_val, "}}");
        if (!tag_end) {
            /* Malformed include directive, append remainder and break */
            buf_append_str(&pass1, inc_start);
            break;
        }

        size_t inc_name_len = (size_t)(tag_end - tag_val);
        char inc_filename[128];
        if (inc_name_len < sizeof(inc_filename)) {
            memcpy(inc_filename, tag_val, inc_name_len);
            inc_filename[inc_name_len] = '\0';

            char *included_content = render_template_internal(inc_filename, ctx, depth + 1);
            if (included_content) {
                buf_append_str(&pass1, included_content);
                free(included_content);
            }
        }

        p = tag_end + 2; /* Skip closing "}}" */
    }
    free(raw);

    /* Pass 2: Substitute {{variable_name}} */
    DynamicBuffer pass2;
    if (!buf_init(&pass2, pass1.len + 128)) {
        buf_free(&pass1);
        return NULL;
    }

    p = pass1.data;
    while (*p != '\0') {
        const char *var_start = strstr(p, "{{");
        if (!var_start) {
            buf_append_str(&pass2, p);
            break;
        }

        /* Append text prior to variable tag */
        buf_append_len(&pass2, p, (size_t)(var_start - p));

        const char *tag_val = var_start + 2;
        const char *tag_end = strstr(tag_val, "}}");
        if (!tag_end) {
            buf_append_str(&pass2, var_start);
            break;
        }

        size_t var_name_len = (size_t)(tag_end - tag_val);
        char var_name[MAX_VAR_KEY_LEN];
        if (var_name_len < sizeof(var_name)) {
            memcpy(var_name, tag_val, var_name_len);
            var_name[var_name_len] = '\0';

            const char *val = template_get(ctx, var_name);
            buf_append_str(&pass2, val);
        }

        p = tag_end + 2;
    }

    buf_free(&pass1);
    return pass2.data;
}

/* Render template file from templates directory with variable substitutions */
char *render_template(const char *filename, TemplateContext *ctx) {
    return render_template_internal(filename, ctx, 0);
}
