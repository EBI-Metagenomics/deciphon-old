#include "dcp/cmp.h"
#include "third-party/cmp.h"
#include <assert.h>
#include <limits.h>
#include <stdbool.h>

static bool file_reader(struct dcp_cmp *cmp, void *data, size_t limit)
{
    FILE *fh = (FILE *)cmp->buf;
    return fread(data, sizeof(uint8_t), limit, fh) == (limit * sizeof(uint8_t));
}

static bool file_skipper(struct dcp_cmp *cmp, size_t count)
{
    assert(count <= ULONG_MAX);
    return fseek((FILE *)cmp->buf, (long)count, SEEK_CUR);
}

static size_t file_writer(struct dcp_cmp *cmp, const void *data, size_t count)
{
    return fwrite(data, sizeof(uint8_t), count, (FILE *)cmp->buf);
}

struct dcp_cmp dcp_cmp_init(FILE *fd)
{
    struct dcp_cmp cmp = {0};
    cmp_init(&cmp, fd, file_reader, file_skipper, file_writer);
    return cmp;
}
