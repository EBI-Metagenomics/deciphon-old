#include "dcp_cmp.h"
#include "third-party/cmp.h"
#include <assert.h>
#include <limits.h>
#include <stdbool.h>

bool cmp_read(struct cmp_ctx_s *cmp, void *data, size_t limit)
{
    FILE *fh = (FILE *)cmp->buf;
    return fread(data, sizeof(uint8_t), limit, fh) == (limit * sizeof(uint8_t));
}

bool cmp_skip(struct cmp_ctx_s *cmp, size_t count)
{
    assert(count <= ULONG_MAX);
    return fseek((FILE *)cmp->buf, (long)count, SEEK_CUR);
}

size_t cmp_write(struct cmp_ctx_s *cmp, const void *data, size_t count)
{
    return fwrite(data, sizeof(uint8_t), count, (FILE *)cmp->buf);
}

void cmp_setup(struct cmp_ctx_s *cmp, FILE *fp)
{
    cmp_init(cmp, fp, cmp_read, cmp_skip, cmp_write);
}
