#include "loop/stdpipe.h"
#include "core/global.h"
#include "core/logy.h"
#include <assert.h>
#include <unistd.h>

void stdpipe_init(struct stdpipe *pipe, struct uv_loop_s *loop,
                  on_exit2_fn_t *on_exit, void *arg)
{
    if (uv_pipe_init(loop, &pipe->uvpipe, 0)) fatal("uv_pipe_init");
    pipe->fd = -1;
    pipe->on_exit = on_exit;
    pipe->arg = arg;
    pipe->no_close = false;
}

void stdpipe_open(struct stdpipe *pipe, int fd)
{
    assert(fd == STDIN_FILENO || fd == STDOUT_FILENO || fd == STDERR_FILENO);
    if (uv_pipe_open(&pipe->uvpipe, fd)) fatal("uv_pipe_open");
    pipe->fd = fd;
    pipe->no_close = false;
}

void stdpipe_close(struct stdpipe *pipe)
{
    if (pipe->no_close) return;
    pipe->no_close = true;

    uv_close((struct uv_handle_s *)&pipe->uvpipe, NULL);
    if (pipe->on_exit) (*pipe->on_exit)(pipe->arg);
    pipe->fd = -1;
}
