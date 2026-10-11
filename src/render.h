#ifndef RENDER_H
#define RENDER_H

#include <stddef.h>

#define MAX_TEMPLATE_VARS 128
#define MAX_VAR_KEY_LEN 128
#define MAX_VAR_VAL_LEN 65536

typedef struct {
    char key[MAX_VAR_KEY_LEN];
    char *value; // heap-allocated
} TemplateVar;

typedef struct {
    TemplateVar vars[MAX_TEMPLATE_VARS];
    int count;
} TemplateContext;

void template_context_init(TemplateContext *ctx);
void template_set(TemplateContext *ctx, const char *key, const char *value);
const char *template_get(const TemplateContext *ctx, const char *key);
void template_context_free(TemplateContext *ctx);

/* Load template from templates/ directory, process includes and variable
 * substitution, return heap-allocated HTML string. Caller must free(). */
char *render_template(const char *filename, TemplateContext *ctx);

/* Load a raw file from disk, return heap-allocated content. Caller must free(). */
char *load_file(const char *filepath, size_t *out_size);

#endif
