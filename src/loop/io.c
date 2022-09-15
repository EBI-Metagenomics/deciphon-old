#include "loop/io.h"
#include "core/c23.h"
#include "core/logging.h"
#include <unistd.h>

void io_init(struct io *io, struct uv_loop_s *loop, io_onopen_fn_t *onopen_fn,
             io_onclose_fn_t *onclose_fn)
{
    io->loop = loop;
    io->onopen_cb = onopen_fn;
    io->onclose_cb = onclose_fn;
}

static void try_call_user_onopen(struct io *io)
{
    bool ok = io->input.fd != -1 && io->output.fd != -1;
    if (io->input.is_open && io->input.is_open) (*io->onopen_cb)(ok);
}

static void onopen_input(struct uv_fs_s *fs)
{
    struct io *io = fs->data;
    if (io->input.fd == -1)
        eio("failed to open input");
    else
        io->input.is_open = true;

    try_call_user_onopen(io);
}

static void onopen_output(struct uv_fs_s *fs)
{
    struct io *io = fs->data;
    if (io->output.fd == -1)
        eio("failed to open output");
    else
        io->output.is_open = true;

    try_call_user_onopen(io);
}

void io_setup(struct io *io, char const *input, char const *output)
{
    io->input.is_open = false;
    io->output.is_open = false;
    io->input.is_over = false;
    io->output.is_over = false;

    io->input.req.data = io;
    io->output.req.data = io;

    if (input)
    {
        io->input.fd = uv_fs_open(io->loop, &io->input.req, input, O_RDWR, 0,
                                  onopen_input);
    }
    else
    {
        io->input.fd = STDIN_FILENO;
        io->input.is_open = true;
    }

    if (output)
    {
        io->output.fd = uv_fs_open(io->loop, &io->output.req, output, O_RDWR, 0,
                                   onopen_output);
    }
    else
    {
        io->output.fd = STDOUT_FILENO;
        io->output.is_open = true;
    }
    try_call_user_onopen(io);
}

static void try_call_user_onclose(struct io *io)
{
    if (io->input.is_over && io->input.is_over) (*io->onclose_cb)();
}

static void onclose_input(struct uv_fs_s *fs)
{
    struct io *io = fs->data;
    uv_fs_req_cleanup(&io->input.req);
    io->input.fd = -1;
    io->input.is_over = true;
    try_call_user_onclose(io);
}

static void onclose_output(struct uv_fs_s *fs)
{
    struct io *io = fs->data;
    uv_fs_req_cleanup(&io->output.req);
    io->output.fd = -1;
    io->output.is_over = true;
    try_call_user_onclose(io);
}

void io_close(struct io *io)
{
    if (io->input.fd != -1 && io->input.fd != STDIN_FILENO)
        uv_fs_close(io->loop, &io->input.req, io->input.fd, onclose_input);
    else
        io->input.is_over = true;

    if (io->output.fd != -1 && io->output.fd != STDOUT_FILENO)
        uv_fs_close(io->loop, &io->output.req, io->output.fd, onclose_output);
    else
        io->output.is_over = true;
}
