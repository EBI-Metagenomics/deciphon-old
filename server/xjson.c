#include "xjson.h"
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
