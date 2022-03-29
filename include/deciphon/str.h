#ifndef DECIPHON_STR_H
#define DECIPHON_STR_H

#include <stdbool.h>
#include <stddef.h>

struct buff;

struct str
{
    struct buff *buff;
};

bool str_init(struct str *, size_t capacity);
bool str_ensure(struct str *, size_t capacity);
bool str_append(struct str *, char const *cstr);
bool str_append_chars(struct str *, size_t nchars, char const *chars);
size_t str_size(struct str const *str);
void str_reset(struct str *str);
void str_cleanup(struct str const *);

#endif
