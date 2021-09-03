#ifndef XCMP_H
#define XCMP_H

#include "third-party/cmp.h"

void xcmp_init(cmp_ctx_t *cmp, FILE *file);

#define xcmp_write_imm_float(ctx, v)                                           \
    _Generic((v), float : cmp_write_float, double : cmp_write_double)(ctx, v)

#define xcmp_read_imm_float(ctx, v)                                            \
    _Generic((v), float * : cmp_read_float, double * : cmp_read_double)(ctx, v)

#endif
