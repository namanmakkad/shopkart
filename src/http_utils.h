#ifndef HTTP_UTILS_H
#define HTTP_UTILS_H

#include <stddef.h>

#define MAX_PARAMS 64

typedef struct {
    char key[256];
    char value[2048];
} KeyValue;

typedef struct {
    KeyValue items[MAX_PARAMS];
    int count;
} ParamList;

void url_decode(const char *src, char *dst, size_t dst_size);
void parse_query_string(const char *qs, ParamList *params);
void parse_form_body(const char *body, ParamList *params);
const char *param_get(const ParamList *params, const char *key);
void html_escape(const char *src, char *dst, size_t dst_size);

#endif
