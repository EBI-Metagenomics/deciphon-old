#ifndef ILOG2_H
#define ILOG2_H

#include "bitops.h"

/**
 * ilog2 - log base 2 of 32-bit or a 64-bit unsigned value
 * @n: parameter
 *
 * constant-capable log of base 2 calculation
 * - this can be used to initialise global variables from constant data, hence
 * the massive ternary operator construction
 *
 * selects the appropriately-sized optimised version depending on sizeof(n)
 */
#define ilog2(x)                                                               \
    (__builtin_constant_p(x) ? ((x) < 2 ? 0 : 63 - __builtin_clzll(x))         \
     : sizeof(x) <= 4        ? ilog2_u32((uint32_t)(x))                                    \
                             : ilog2_u64(x))

static inline unsigned ilog2_u32(uint32_t n) { return bitops_fls32(n) - 1; }

static inline unsigned ilog2_u64(uint64_t n) { return bitops_fls64(n) - 1; }

#endif
