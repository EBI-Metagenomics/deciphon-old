#include "xjson.h"
#include "deciphon/to.h"
#include "jsmn_error.h"
#include <string.h>

static inline unsigned tok_size(jsmntok_t const *tok)
{
    return (unsigned)(tok->end - tok->start);
}

static inline char const *tok_value(jsmntok_t const *tok, char const *data)
{
    return data + tok->start;
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

enum rc xjson_parse(struct xjson *x, char const *data, unsigned size)
{
    jsmn_init(&x->parser);
    x->size = size;
    x->data = data;
    int r = jsmn_parse(&x->parser, x->data, x->size, x->tok, XJSON_MAX_TOKENS);
    x->ntoks = (unsigned)r;
    return r < 0 ? jsmn_error(r) : RC_OK;
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
