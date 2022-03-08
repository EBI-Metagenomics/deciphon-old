#include "xjson.h"
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
    if (r < 0) jsmn_error(r);
    return RC_OK;
}

// enum jsmnerr
// {
//     /* Not enough tokens were provided */
//     JSMN_ERROR_NOMEM = -1,
//     /* Invalid character inside JSON string */
//     JSMN_ERROR_INVAL = -2,
//     /* The string is not a full JSON packet, more bytes expected */
//     JSMN_ERROR_PART = -3
// };
