#include "file.h"
#include "imm/imm.h"
#include "support.h"
#include <limits.h>

static void __cmp_init(cmp_ctx_t *cmp, FILE *file);

int file_close(struct file *f)
{
    if (!f->fd)
        return IMM_SUCCESS;

    if (fclose(f->fd))
    {
        f->fd = NULL;
        return error(IMM_IOERROR, "could not close file");
    }
    return IMM_SUCCESS;
}

int file_open(struct file *f, const char *restrict filepath,
              const char *restrict mode)
{
    if (!(f->fd = fopen(filepath, mode)))
    {
        f->fd = NULL;
        return error(IMM_IOERROR, "could not open file %s (%s)", filepath,
                     mode);
    }

    __cmp_init(&f->ctx, f->fd);

    f->mode = 0;
    if (mode[0] == 'w')
        f->mode |= FILE_WRIT;
    if (mode[0] == 'r')
        f->mode |= FILE_READ;
    if (strlen(mode) > 1 && mode[1] == '+')
        f->mode |= FILE_READ;

    return IMM_SUCCESS;
}

int file_rewind(struct file *f)
{
    if (fseek(f->fd, 0L, SEEK_SET))
        return error(IMM_IOERROR, "failed to fseek");
    return IMM_SUCCESS;
}

static bool file_reader(cmp_ctx_t *ctx, void *data, size_t limit)
{
    FILE *fh = (FILE *)ctx->buf;
    return fread(data, sizeof(uint8_t), limit, fh) == (limit * sizeof(uint8_t));
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

static void __cmp_init(cmp_ctx_t *cmp, FILE *file)
{
    cmp_init(cmp, file, file_reader, file_skipper, file_writer);
}
