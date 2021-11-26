#include "xcmp.h"
#include "third-party/cmp.h"
#include <assert.h>
#include <limits.h>
#include <stdbool.h>

bool xcmp_read(struct cmp_ctx_s *cmp, void *data, size_t limit)
{
    FILE *fh = (FILE *)cmp->buf;
    return fread(data, sizeof(uint8_t), limit, fh) == (limit * sizeof(uint8_t));
}

bool xcmp_skip(struct cmp_ctx_s *cmp, size_t count)
{
    assert(count <= ULONG_MAX);
    return fseek((FILE *)cmp->buf, (long)count, SEEK_CUR);
}

size_t xcmp_write(struct cmp_ctx_s *cmp, const void *data, size_t count)
{
    return fwrite(data, sizeof(uint8_t), count, (FILE *)cmp->buf);
}

struct cmp_ctx_s xcmp_init(FILE *fp)
{
    struct cmp_ctx_s cmp = {0};
    cmp_init(&cmp, fp, xcmp_read, xcmp_skip, xcmp_write);
    return cmp;
}

void xcmp_setup(struct cmp_ctx_s *cmp, FILE *fp)
{
    cmp->error = 0;
    cmp->buf = fp;
    cmp->read = xcmp_read;
    cmp->skip = xcmp_skip;
    cmp->write = xcmp_write;
}
