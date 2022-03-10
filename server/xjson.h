#ifndef XJSON_H
#define XJSON_H

#define JSMN_STATIC
#include "jsmn.h"
#include <stdbool.h>
#include <stdint.h>

#define XJSON_MAX_TOKENS 128

struct xjson
{
    jsmn_parser parser;
    struct jsmntok tok[XJSON_MAX_TOKENS];
    unsigned ntoks;
    unsigned size;
    char const *data;
};

enum rc xjson_parse(struct xjson *x, char const *data, unsigned size);

unsigned json_tok_size(struct xjson const *x, unsigned idx);

char const *json_tok_value(struct xjson const *x, unsigned idx);

bool xjson_eqstr(struct xjson const *x, unsigned idx, char const *str);

static inline bool xjson_is_number(struct xjson const *x, unsigned idx)
{
    struct jsmntok const *tok = x->tok + idx;
    char c = x->data[x->tok[idx].start];
    return tok->type == JSMN_PRIMITIVE && c != 'n' && c != 't' && c != 'f';
}

static bool xjson_is_bool(struct xjson const *x, unsigned idx)
{
    struct jsmntok const *tok = x->tok + idx;
    char c = x->data[tok->start];
    return tok->type == JSMN_PRIMITIVE && (c == 't' || c == 'f');
}

static inline bool xjson_is_string(struct xjson const *x, unsigned idx)
{
    struct jsmntok const *tok = x->tok + idx;
    return tok->type == JSMN_STRING;
}

static bool xjson_to_bool(struct xjson const *x, unsigned idx)
{
    return x->data[x->tok[idx].start] == 't';
}

enum rc xjson_bind_int64(struct xjson *x, unsigned idx, int64_t *dst);

enum rc xjson_copy_str(struct xjson *x, unsigned idx, unsigned dst_size,
                       char *dst);

#endif
