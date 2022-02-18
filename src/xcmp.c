#include "xcmp.h"
#include "buff.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

static_assert(sizeof(size_t) >= 8, "64-bits");
static_assert(sizeof(off_t) >= 8, "64-bits");

static bool read(struct lip_io_file *ctx, void *data, size_t sz)
{
    struct xcmp *x = ctx->buf;
    memcpy(data, x->read.buff + x->read.pos, sz);
    x->read.pos += sz;
    return true;
}

// static bool skip(struct lip_io_file *ctx, size_t count)
// {
//     return fseeko((FILE *)ctx->buf, (off_t)count, SEEK_CUR) != -1;
// }

static size_t write(struct lip_io_file *ctx, const void *data, size_t count)
{
    struct xcmp *x = ctx->buf;
    return fwrite(data, sizeof(uint8_t), count, x->fp);
}

bool xcmp_setup(struct xcmp *x, FILE *fp, enum xcmp_mode mode)
{
    x->fp = fp;
    struct buff *buff = buff_new(1);
    x->read.buff = buff;
    if (!buff) return false;
    cmp_init(&x->cmp, x, read, 0, write);
    x->read.base = ftello(x->fp);
    x->read.pos = 0;
    x->read.fpos = x->read.base;
    return true;
}

bool xcmp_load(struct xcmp *x, size_t size)
{
    assert(x->mode == XCMP_READ);
    if (!(x->read.buff = buff_ensure(x->read.buff, size))) return false;
    x->read.base = x->read.fpos;
    x->read.pos = 0;
    bool ok =
        fread(x->read.buff->data, 1, size, x->fp) == (unsigned long)(size);
    x->read.fpos = x->read.base + (off_t)size;
    return ok;
}

void xcmp_del(struct xcmp *x)
{
    if (x->mode == XCMP_READ && x->read.buff) buff_del(x->read.buff);
}
