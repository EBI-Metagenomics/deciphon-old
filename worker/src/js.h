#ifndef JS_H
#define JS_H

#include "common/compiler.h"
#include <stdbool.h>
#include <stdint.h>

struct lip_io_file;

bool js_write_int(struct lip_io_file *ctx, int64_t u);
bool js_write_uint(struct lip_io_file *ctx, uint64_t u);

bool js_read_i64(struct lip_io_file *ctx, int64_t *u);
bool js_read_i32(struct lip_io_file *ctx, int32_t *u);
bool js_read_i16(struct lip_io_file *ctx, int16_t *u);
bool js_read_i8(struct lip_io_file *ctx, int8_t *u);

bool js_read_u64(struct lip_io_file *ctx, uint64_t *u);
bool js_read_u32(struct lip_io_file *ctx, uint32_t *u);
bool js_read_u16(struct lip_io_file *ctx, uint16_t *u);
bool js_read_u8(struct lip_io_file *ctx, uint8_t *u);

bool js_write_str(struct lip_io_file *ctx, char const *str, uint32_t size);
#define JS_WRITE_STR(ctx, msg)                                                 \
    js_write_str(ctx, __STRADDR(msg), (uint32_t)(__STRSIZE(msg)))

bool js_read_str_size(struct lip_io_file *ctx, uint32_t *size);
bool js_read_str(struct lip_io_file *ctx, char *str, uint32_t *size);

bool js_xpec_str(struct lip_io_file *ctx, char const *str, uint32_t size);
#define JS_XPEC_STR(ctx, msg)                                                  \
    js_xpec_str(ctx, __STRADDR(msg), (uint32_t)(__STRSIZE(msg)))

bool js_write_map(struct lip_io_file *ctx, uint32_t size);
bool js_xpec_map(struct lip_io_file *ctx, uint32_t size);

#endif
