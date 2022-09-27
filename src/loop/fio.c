#include "loop/fio.h"
#include "core/c23.h"
#include "core/logging.h"
#include <unistd.h>

static void onopen(struct uv_fs_s *fs);
static void onclose(struct uv_fs_s *fs);

void fio_init(struct fio *io, struct uv_loop_s *loop,
              fio_onopen_fn_t *onopen_cb, fio_onclose_fn_t *onclose_cb,
              void *arg)
{
    io->loop = loop;
    io->onopen_cb = onopen_cb;
    io->onclose_cb = onclose_cb;
    io->arg = arg;
    io->fd = -1;
    io->nofs_close = false;
    io->closed = false;
}

void fio_open(struct fio *io, char const *file, int flags, int mode)
{
    io->closed = false;
    io->req.data = io;

    if (!strcmp(file, "&1"))
    {
        io->fd = STDIN_FILENO;
        (*io->onopen_cb)(true, io->arg);
        io->nofs_close = true;
    }
    else if (!strcmp(file, "&2"))
    {
        io->fd = STDOUT_FILENO;
        (*io->onopen_cb)(true, io->arg);
        io->nofs_close = true;
    }
    else
    {
        uv_fs_open(io->loop, &io->req, file, flags, mode, onopen);
        io->nofs_close = false;
    }
}

void fio_close(struct fio *io)
{
    if (io->closed) return;
    if (io->nofs_close)
    {
        io->closed = true;
        (*io->onclose_cb)(io->arg);
    }
    else
        uv_fs_close(io->loop, &io->req, io->fd, onclose);
}

int fio_fd(struct fio const *io) { return io->fd; }

bool fio_isclosed(struct fio const *io) { return io->closed; }

static void onopen(struct uv_fs_s *fs)
{
    struct fio *io = fs->data;
    io->fd = fs->result;
    if (io->fd < -1) fatal(uv_strerror(fs->result));
    (*io->onopen_cb)(io->fd != -1, io->arg);
}

static void onclose(struct uv_fs_s *fs)
{
    struct fio *io = fs->data;
    io->closed = true;
    uv_fs_req_cleanup(&io->req);
    io->fd = -1;
    (*io->onclose_cb)(io->arg);
}
