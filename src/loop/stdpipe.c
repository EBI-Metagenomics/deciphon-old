#include "loop/stdpipe.h"
#include "core/global.h"
#include "core/logy.h"
#include <assert.h>

void stdpipe_init(struct stdpipe *pipe, struct uv_loop_s *loop)
{
    if (uv_pipe_init(loop, &pipe->uvpipe, 0)) fatal("uv_pipe_init");
    pipe->uvpipe.data = pipe;
    pipe->fd = -1;
}

void stdpipe_open(struct stdpipe *pipe, int fd)
{
    if (uv_pipe_open(&pipe->uvpipe, fd)) fatal("uv_pipe_open");
    pipe->fd = fd;
}

void stdpipe_close(struct stdpipe *pipe)
{
    uv_close((struct uv_handle_s *)&pipe->uvpipe, NULL);
    pipe->fd = -1;
}
