#ifndef IMATH_H
#define IMATH_H

#include "imm/imm.h"

static inline uint32_t imath_ceildiv(uint32_t x, uint32_t y)
{
    IMM_BUG(y == 0);
    return (x + (y - 1)) / y;
}

#endif
