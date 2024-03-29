#include "xjson.h"
#include "deciphon/core/logging.h"
#include "deciphon/core/to.h"
#include "jsmn_error.h"
#include <assert.h>
#include <string.h>

static inline unsigned tok_size(jsmntok_t const *tok)
{
    return (unsigned)(tok->end - tok->start);
}

static inline char const *tok_value(jsmntok_t const *tok, char const *data)
{
    return data + tok->start;
}

unsigned json_tok_size(struct xjson const *x, unsigned idx)
{
    return tok_size(x->tok + idx);
}

enum rc xjson_parse(struct xjson *x, char const *data, unsigned size)
{
    jsmn_init(&x->parser);
    x->size = size;
    x->data = data;
    int r = jsmn_parse(&x->parser, x->data, x->size, x->tok, XJSON_MAX_TOKENS);
    x->ntoks = (unsigned)r;
    return r < 0 ? jsmn_error(r) : RC_OK;
}

char const *json_tok_value(struct xjson const *x, unsigned idx)
{
    return tok_value(x->tok + idx, x->data);
}

bool xjson_eqstr(struct xjson const *x, unsigned idx, char const *str)
{
    unsigned len = (unsigned)strlen(str);

    struct jsmntok const *tok = x->tok + idx;
    jsmntype_t type = tok->type;
    unsigned size = tok_size(tok);
    char const *value = tok_value(tok, x->data);

    return type == JSMN_STRING && len == size && !strncmp(value, str, size);
}

bool xjson_is_array(struct xjson const *x, unsigned idx)
{
    struct jsmntok const *tok = x->tok + idx;
    return tok->type == JSMN_ARRAY;
}

bool xjson_is_array_empty(struct xjson const *x, unsigned idx)
{
    (void)idx;
    assert(xjson_is_array(x, idx));
    return x->ntoks == 1;
}

bool xjson_is_number(struct xjson const *x, unsigned idx)
{
    struct jsmntok const *tok = x->tok + idx;
    char c = x->data[x->tok[idx].start];
    return tok->type == JSMN_PRIMITIVE && c != 'n' && c != 't' && c != 'f';
}

bool xjson_is_bool(struct xjson const *x, unsigned idx)
{
    struct jsmntok const *tok = x->tok + idx;
    char c = x->data[tok->start];
    return tok->type == JSMN_PRIMITIVE && (c == 't' || c == 'f');
}

bool xjson_is_string(struct xjson const *x, unsigned idx)
{
    struct jsmntok const *tok = x->tok + idx;
    return tok->type == JSMN_STRING;
}

bool xjson_to_bool(struct xjson const *x, unsigned idx)
{
    return x->data[x->tok[idx].start] == 't';
}

enum rc xjson_bind_int(struct xjson *x, unsigned idx, int *dst)
{
    if (!xjson_is_number(x, idx)) return einval("expected number");
    struct jsmntok const *tok = x->tok + idx;
    unsigned size = tok_size(tok);
    char const *value = tok_value(tok, x->data);
    if (!to_intl(size, value, dst)) return eparse("parse number");
    return RC_OK;
}

enum rc xjson_bind_int32(struct xjson *x, unsigned idx, int32_t *dst)
{
    if (!xjson_is_number(x, idx)) return einval("expected number");
    struct jsmntok const *tok = x->tok + idx;
    unsigned size = tok_size(tok);
    char const *value = tok_value(tok, x->data);
    if (!to_int32l(size, value, dst)) return eparse("parse number");
    return RC_OK;
}

enum rc xjson_bind_int64(struct xjson *x, unsigned idx, int64_t *dst)
{
    if (!xjson_is_number(x, idx)) return einval("expected number");
    struct jsmntok const *tok = x->tok + idx;
    unsigned size = tok_size(tok);
    char const *value = tok_value(tok, x->data);
    if (!to_int64l(size, value, dst)) return eparse("parse number");
    return RC_OK;
}

enum rc xjson_copy_str(struct xjson *x, unsigned idx, unsigned dst_size,
                       char *dst)
{
    if (!xjson_is_string(x, idx)) return einval("expected string");

    struct jsmntok const *tok = x->tok + idx;
    unsigned size = tok_size(tok);

    if (size >= dst_size) return enomem("string is too long");

    memcpy(dst, tok_value(tok, x->data), size);
    dst[size] = 0;
    return RC_OK;
}
