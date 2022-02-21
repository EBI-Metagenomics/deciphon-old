#include "js.h"
#include <assert.h>
#include <stdint.h>

bool js_write_int(struct lip_file *ctx, int64_t u)
{
    return cmp_write_integer(ctx, u);
}

bool js_write_uint(struct lip_file *ctx, uint64_t u)
{
    return cmp_write_uinteger(ctx, u);
}

bool js_read_i64(struct lip_file *ctx, int64_t *u)
{
    int64_t d = 0;
    if (!cmp_read_integer(ctx, &d)) return false;
    *u = d;
    return true;
}

bool js_read_i32(struct lip_file *ctx, int32_t *u)
{
    int64_t d = 0;
    if (!cmp_read_integer(ctx, &d)) return false;
    *u = (int32_t)d;
    return d <= INT32_MAX;
}

bool js_read_i16(struct lip_file *ctx, int16_t *u)
{
    int64_t d = 0;
    if (!cmp_read_integer(ctx, &d)) return false;
    *u = (int16_t)d;
    return d <= INT16_MAX;
}

bool js_read_i8(struct lip_file *ctx, int8_t *u)
{
    int64_t d = 0;
    if (!cmp_read_integer(ctx, &d)) return false;
    *u = (int8_t)d;
    return d <= INT8_MAX;
}

bool js_read_u64(struct lip_file *ctx, uint64_t *u)
{
    uint64_t d = 0;
    if (!cmp_read_uinteger(ctx, &d)) return false;
    *u = d;
    return true;
}

bool js_read_u32(struct lip_file *ctx, uint32_t *u)
{
    uint64_t d = 0;
    if (!cmp_read_uinteger(ctx, &d)) return false;
    *u = (uint32_t)d;
    return d <= UINT32_MAX;
}

bool js_read_u16(struct lip_file *ctx, uint16_t *u)
{
    uint64_t d = 0;
    if (!cmp_read_uinteger(ctx, &d)) return false;
    *u = (uint16_t)d;
    return d <= UINT16_MAX;
}

bool js_read_u8(struct lip_file *ctx, uint8_t *u)
{
    uint64_t d = 0;
    if (!cmp_read_uinteger(ctx, &d)) return false;
    *u = (uint8_t)d;
    return d <= UINT8_MAX;
}

static inline void make_sure_cstring(char const *str, uint32_t size)
{
    for (uint32_t i = 0; i < size; ++i)
        assert(str[i]);
}

bool js_write_str(struct lip_file *ctx, char const *str, uint32_t size)
{
    make_sure_cstring(str, size);
    return cmp_write_str(ctx, str, size);
}

bool js_read_str_size(struct lip_file *ctx, uint32_t *size)
{
    return cmp_read_str_size(ctx, size);
}

bool js_read_str(struct lip_file *ctx, char *str, uint32_t *size)
{
    bool ok = cmp_read_str(ctx, str, size);
    make_sure_cstring(str, *size);
    return ok;
}

bool js_xpec_str(struct lip_file *ctx, char const *str, uint32_t size)
{
    uint32_t sz = 0;
    if (!js_read_str_size(ctx, &sz)) return false;
    if (sz != size) return false;

    int8_t c = 0;
    for (unsigned i = 0; i < sz; ++i)
    {
        if (!cmp_read_char(ctx, &c)) return false;
        assert(c);
        if (c != str[i]) return false;
    }
    return str[sz] == 0;
}

bool js_write_map(struct lip_file *ctx, uint32_t size)
{
    return cmp_write_map(ctx, size);
}

bool js_xpec_map(struct lip_file *ctx, uint32_t size)
{
    uint32_t u32 = 0;
    if (!cmp_read_map(ctx, &u32)) return false;
    return u32 == size;
}
