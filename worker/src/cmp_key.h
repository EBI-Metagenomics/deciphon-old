#ifndef CMP_KEY_H
#define CMP_KEY_H

#include "cmp/cmp.h"
#include "common/compiler.h"

static inline bool cmp_key_skip(struct cmp_ctx_s *cmp, uint32_t len)
{
    uint32_t l = 0;
    if (!__cmp_read_str_size(cmp, &l)) return false;
    if (l != len) return false;
    return cmp_skip(cmp, len);
}

#define CMP_KEY_SKIP(cmp, key) cmp_key_skip(cmp, (uint32_t)(__ARRSIZE(key) - 1))

#endif
