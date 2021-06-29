#ifndef IO_H
#define IO_H

#include "cmp.h"
#include "support.h"
#include <limits.h>

static bool __file_reader(cmp_ctx_t *ctx, void *data, size_t limit)
{
    FILE *fh = (FILE *)ctx->buf;
    return fread(data, sizeof(uint8_t), limit, fh) == (limit * sizeof(uint8_t));
}

static bool __file_skipper(cmp_ctx_t *ctx, size_t count)
{
    IMM_BUG(count > ULONG_MAX);
    return fseek((FILE *)ctx->buf, (long)count, SEEK_CUR);
}

static size_t __file_writer(cmp_ctx_t *ctx, const void *data, size_t count)
{
    return fwrite(data, sizeof(uint8_t), count, (FILE *)ctx->buf);
}

static inline void io_init(cmp_ctx_t *cmp, FILE *file)
{
    cmp_init(cmp, file, __file_reader, __file_skipper, __file_writer);
}

#endif
