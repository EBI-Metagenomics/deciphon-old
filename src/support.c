#include "support.h"
#include "error.h"
#include <assert.h>
#include <limits.h>

#define BUFFSIZE (8 * 1024)

enum dcp_rc fcopy(FILE *restrict dst, FILE *restrict src)
{
    char buffer[BUFFSIZE];
    size_t n = 0;
    while ((n = fread(buffer, sizeof(*buffer), BUFFSIZE, src)) > 0)
    {
        if (n < BUFFSIZE && ferror(src))
            return error(DCP_IOERROR, "failed to read file");

        if (fwrite(buffer, sizeof(*buffer), n, dst) < n)
            return error(DCP_IOERROR, "failed to write file");
    }
    if (ferror(src))
        return error(DCP_IOERROR, "failed to read file");

    return DCP_SUCCESS;
}

static bool file_reader(cmp_ctx_t *ctx, void *data, size_t limit)
{
    FILE *fh = (FILE *)ctx->buf;
    return fread(data, sizeof(uint8_t), limit, fh) == (limit * sizeof(uint8_t));
}

static bool file_skipper(cmp_ctx_t *ctx, size_t count)
{
    assert(count <= ULONG_MAX);
    return fseek((FILE *)ctx->buf, (long)count, SEEK_CUR);
}

static size_t file_writer(cmp_ctx_t *ctx, const void *data, size_t count)
{
    return fwrite(data, sizeof(uint8_t), count, (FILE *)ctx->buf);
}

void xcmp_init(cmp_ctx_t *cmp, FILE *file)
{
    cmp_init(cmp, file, file_reader, file_skipper, file_writer);
}
