#ifndef XJSON_H
#define XJSON_H

#include "jsmn.h"
#include <stdbool.h>

#define XJSON_MAX_TOKENS 128

struct xjson
{
    jsmn_parser parser;
    struct jsmntok tok[XJSON_MAX_TOKENS];
    unsigned ntoks;
    char const *data;
};

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

static bool xjson_to_bool(struct xjson const *x, unsigned idx)
{
    return x->data[x->tok[idx].start] == 't';
}

#endif
