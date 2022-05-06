#ifndef DECIPHON_CORE_HASH_H
#define DECIPHON_CORE_HASH_H

#include <stdint.h>

#define __CORE_HASH_UNSIGNED(x)                                                \
    _Generic((x), char                                                         \
             : (unsigned char)(x), signed char                                 \
             : (unsigned char)(x), short                                       \
             : (unsigned short)(x), int                                        \
             : (unsigned int)(x), long                                         \
             : (unsigned long)(x), long long                                   \
             : (unsigned long long)(x), default                                \
             : (unsigned int)(x))

#define __CORE_HASH_GOLDEN_RATIO_32 0x61C88647
#define __CORE_HASH_GOLDEN_RATIO_64 0x61C8864680B583EBull

static inline uint32_t core_hash_32(uint32_t val)
{
    return val * __CORE_HASH_GOLDEN_RATIO_32;
}

static inline uint32_t core_hash_64(uint64_t val)
{
    return (uint32_t)(val * __CORE_HASH_GOLDEN_RATIO_64);
}

/* Use core_hash_32 when possible to allow for fast 32bit hashing in 64bit
 * kernels.
 */
#define core_hash(x)                                                           \
    (sizeof(x) <= 4 ? core_hash_32(__CORE_HASH_UNSIGNED(x))                    \
                    : core_hash_64(__CORE_HASH_UNSIGNED(x)))

#endif
