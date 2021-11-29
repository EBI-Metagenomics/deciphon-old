#ifndef XCMP_H
#define XCMP_H

/* Wrapper around [CMP library](https://github.com/camgunz/cmp). */

#include "third-party/cmp.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

struct cmp_ctx_s;

bool xcmp_read(struct cmp_ctx_s *, void *data, size_t limit);
bool xcmp_skip(struct cmp_ctx_s *, size_t count);
size_t xcmp_write(struct cmp_ctx_s *, const void *data, size_t count);

struct cmp_ctx_s xcmp_init(FILE *fp);
void xcmp_setup(struct cmp_ctx_s *cmp, FILE *fp);

static inline FILE *xcmp_fp(struct cmp_ctx_s *cmp) { return (FILE *)cmp->buf; }

static inline int xcmp_close(struct cmp_ctx_s *cmp)
{
    return fclose(xcmp_fp(cmp));
}

static inline void xcmp_rewind(struct cmp_ctx_s *cmp) { rewind(xcmp_fp(cmp)); }

#endif
