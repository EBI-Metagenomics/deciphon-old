#ifndef XJSON_H
#define XJSON_H

#define JSMN_HEADER
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

bool xjson_is_array(struct xjson const *x, unsigned idx);
bool xjson_is_array_empty(struct xjson const *x, unsigned idx);
bool xjson_is_number(struct xjson const *x, unsigned idx);
bool xjson_is_bool(struct xjson const *x, unsigned idx);
bool xjson_is_string(struct xjson const *x, unsigned idx);

bool xjson_to_bool(struct xjson const *x, unsigned idx);

enum rc xjson_bind_int(struct xjson *x, unsigned idx, int *dst);
enum rc xjson_bind_int32(struct xjson *x, unsigned idx, int32_t *dst);
enum rc xjson_bind_int64(struct xjson *x, unsigned idx, int64_t *dst);

enum rc xjson_copy_str(struct xjson *x, unsigned idx, unsigned dst_size,
                       char *dst);

#endif
