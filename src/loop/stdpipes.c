#include "loop/stdpipes.h"
#include "die.h"
#include "loop/global.h"
#include "unused.h"
#include <assert.h>
#include <stdbool.h>
#include <unistd.h>
#include <uv.h>

static struct uv_pipe_s uv[2] = {0};
static on_exit_fn_t *on_exit = NULL;
static bool no_close = true;
static int remain = 2;

void stdpipes_init(on_exit_fn_t *on_exit_cb)
{
    assert(STDIN_FILENO == 0 || STDOUT_FILENO == 1);
    on_exit = on_exit_cb;
    for (int i = 0; i < 2; ++i)
    {
        if (uv_pipe_init(global_loop(), &uv[i], 0)) die();
        if (uv_pipe_open(&uv[i], i)) die();
    }
    no_close = false;
    remain = 2;
}

struct uv_pipe_s *stdpipes_uv(int fd)
{
    assert(STDIN_FILENO == fd || STDOUT_FILENO == fd);
    return &uv[fd];
}

static void on_exit_fwd(struct uv_handle_s *h)
{
    unused(h);
    if (!--remain && on_exit) (*on_exit)();
}

void stdpipes_close(void)
{
    if (no_close) return;
    no_close = true;

    for (int i = 0; i < 2; ++i)
        uv_close((struct uv_handle_s *)&uv[i], &on_exit_fwd);
}
