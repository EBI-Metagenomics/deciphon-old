#ifndef XLIP_H
#define XLIP_H

#include "imm/float.h"
#include "lite_pack/1darray/1darray.h"
#include "lite_pack/io/file.h"
#include "lite_pack/lite_pack.h"
#include <stdbool.h>

#ifdef IMM_DOUBLE_PRECISION
#define XLIP_1DARRAY_FLOAT LIP_1DARRAY_F64
#else
#define XLIP_1DARRAY_FLOAT LIP_1DARRAY_F32
#endif

struct lip_io_file;

bool xlip_expect_map(struct lip_io_file *io, unsigned size);
bool xlip_expect_key(struct lip_io_file *io, char const key[]);
bool xlip_expect_1darray_u8(struct lip_io_file *, unsigned size, uint8_t arr[]);
bool xlip_expect_1darray_u8_type(struct lip_io_file *io, unsigned size,
                                 uint8_t type, uint8_t arr[]);
bool xlip_expect_1darray_float(struct lip_io_file *, unsigned size,
                               imm_float arr[]);
bool xlip_expect_1darray_float_type(struct lip_io_file *io, unsigned size,
                                    uint8_t type, imm_float arr[]);

void xlip_write_cstr(struct lip_io_file *io, char const str[]);
void xlip_read_cstr(struct lip_io_file *io, unsigned size, char str[]);

static inline void xlip_write_key(struct lip_io_file *io, char const key[])
{
    xlip_write_cstr(io, key);
}

#endif
