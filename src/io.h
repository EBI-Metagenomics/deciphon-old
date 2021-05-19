#ifndef IO_H
#define IO_H

#include "cmp.h"
#include "support.h"
#include <assert.h>
#include <limits.h>

static bool read_bytes(void *data, size_t sz, FILE *fh)
{
    return fread(data, sizeof(uint8_t), sz, fh) == (sz * sizeof(uint8_t));
}

static bool file_reader(cmp_ctx_t *ctx, void *data, size_t limit)
{
    return read_bytes(data, limit, (FILE *)ctx->buf);
}

static bool file_skipper(cmp_ctx_t *ctx, size_t count)
{
    IMM_BUG(count > ULONG_MAX);
    return fseek((FILE *)ctx->buf, (long)count, SEEK_CUR);
}

static size_t file_writer(cmp_ctx_t *ctx, const void *data, size_t count)
{
    return fwrite(data, sizeof(uint8_t), count, (FILE *)ctx->buf);
}

#endif
