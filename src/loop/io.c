#include "loop/io.h"
#include "core/c23.h"
#include "core/logging.h"
#include <unistd.h>

static void onopen(struct uv_fs_s *fs);
static void onclose(struct uv_fs_s *fs);

void io_init(struct io *io, struct uv_loop_s *loop, io_onopen_fn_t *onopen_fn,
             io_onclose_fn_t *onclose_fn)
{
    io->loop = loop;
    io->onopen_cb = onopen_fn;
    io->onclose_cb = onclose_fn;
    io->fd = -1;
}

void io_open(struct io *io, char const *file, int mode)
{
    io->req.data = io;

    if (!strcmp(file, "&1"))
    {
        io->fd = STDIN_FILENO;
        (*io->onopen_cb)(true);
    }
    else if (!strcmp(file, "&2"))
    {
        io->fd = STDOUT_FILENO;
        (*io->onopen_cb)(true);
    }
    else
        uv_fs_open(io->loop, &io->req, file, mode, 0, onopen);
}

void io_close(struct io *io)
{
    if (io->fd != -1 && io->fd != STDIN_FILENO && io->fd != STDOUT_FILENO)
        uv_fs_close(io->loop, &io->req, io->fd, onclose);
    else
        (*io->onclose_cb)();
}

static void onopen(struct uv_fs_s *fs)
{
    struct io *io = fs->data;
    io->fd = fs->result;
    if (io->fd == -1) eio("failed to open input");
    (*io->onopen_cb)(io->fd != -1);
}

static void onclose(struct uv_fs_s *fs)
{
    struct io *io = fs->data;
    uv_fs_req_cleanup(&io->req);
    io->fd = -1;
    (*io->onclose_cb)();
}
