#ifndef HELPER_H
#define HELPER_H

#include "imm/imm.h"

static inline imm_float zero(void) { return imm_lprob_zero(); }
static inline char*     fmt_name(char* restrict buffer, char const* name, unsigned i)
{
    sprintf(buffer, "%s%u", name, i);
    return buffer;
}

#endif
