#ifndef DCP_EXPECT_H
#define DCP_EXPECT_H

#include "imm/float.h"
#include <stdbool.h>
#include <stdint.h>

struct lip_file;

bool expect_map(struct lip_file *, unsigned size);
bool expect_key(struct lip_file *, char const key[]);
bool expect_1darray_u8(struct lip_file *, unsigned size, uint8_t arr[]);
bool expect_1darray_u8_type(struct lip_file *, unsigned size, uint8_t type,
                            uint8_t arr[]);
bool expect_1darray_float(struct lip_file *, unsigned size, imm_float arr[]);
bool expect_1darray_float_type(struct lip_file *, unsigned size, uint8_t type,
                               imm_float arr[]);

#endif
