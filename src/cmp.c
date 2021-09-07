#include "dcp/cmp.h"
#include "third-party/cmp.h"
#include <assert.h>
#include <limits.h>
#include <stdbool.h>

static bool file_reader(struct dcp_cmp_ctx *ctx, void *data, size_t limit)
{
    FILE *fh = (FILE *)ctx->buf;
    return fread(data, sizeof(uint8_t), limit, fh) == (limit * sizeof(uint8_t));
}

static bool file_skipper(struct dcp_cmp_ctx *ctx, size_t count)
{
    assert(count <= ULONG_MAX);
    return fseek((FILE *)ctx->buf, (long)count, SEEK_CUR);
}

static size_t file_writer(struct dcp_cmp_ctx *ctx, const void *data,
                          size_t count)
{
    return fwrite(data, sizeof(uint8_t), count, (FILE *)ctx->buf);
}

void dcp_cmp_init(struct dcp_cmp_ctx *cmp, FILE *file)
{
    cmp_init(cmp, file, file_reader, file_skipper, file_writer);
}
