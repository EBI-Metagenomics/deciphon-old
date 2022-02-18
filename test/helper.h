#ifndef HELPER_H
#define HELPER_H

#include "imm/imm.h"

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

static inline imm_float zero(void) { return imm_lprob_zero(); }
static inline char*     fmt_name(char* buffer, char const* name, unsigned i)
{
    sprintf(buffer, "%s%u", name, i);
    return buffer;
}

#endif
