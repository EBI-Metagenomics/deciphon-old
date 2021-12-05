#ifndef DCP_CMP_H
#define DCP_CMP_H

#include "third-party/cmp.h"
#include <stdbool.h>
#include <stdio.h>

struct cmp_ctx_s;

bool cmp_read(struct cmp_ctx_s *, void *data, size_t limit);
bool cmp_skip(struct cmp_ctx_s *, size_t count);
size_t cmp_write(struct cmp_ctx_s *, const void *data, size_t count);

void cmp_setup(struct cmp_ctx_s *cmp, FILE *fp);

static inline FILE *cmp_file(struct cmp_ctx_s *cmp) { return (FILE *)cmp->buf; }

#endif
