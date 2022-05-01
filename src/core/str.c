#include "deciphon/str.h"
#include "deciphon/buff.h"
#include <string.h>

bool str_init(struct str *str, size_t capacity)
{
    if (!(str->buff = buff_new(capacity + 1))) return false;
    str_reset(str);
    return true;
}

bool str_ensure(struct str *str, size_t capacity)
{
    return buff_ensure(&str->buff, capacity + 1);
}

bool str_append(struct str *str, char const *cstr)
{
    return str_append_chars(str, strlen(cstr), cstr);
}

bool str_append_chars(struct str *str, size_t nchars, char const *chars)
{
    if (!str_ensure(str, str->buff->size + nchars + 1)) return false;

    memcpy(str->buff->data + str->buff->size, chars, nchars);
    str->buff->size += nchars;
    str->buff->data[str->buff->size] = '\0';

    return true;
}

size_t str_size(struct str const *str) { return str->buff->size; }

void str_reset(struct str *str)
{
    str->buff->size = 0;
    str->buff->data[0] = '\0';
}

void str_cleanup(struct str const *str) { buff_del(str->buff); }
