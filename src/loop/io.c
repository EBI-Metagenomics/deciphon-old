#include "loop/io.h"
#include "core/c23.h"
#include "core/logging.h"
#include <unistd.h>

void io_init(struct io *io, struct uv_loop_s *loop) { io->loop = loop; }

bool io_setup(struct io *io, char const *input, char const *output)
{
    if (input)
    {
        int fd =
            uv_fs_open(io->loop, &io->input.req, input, O_RDONLY, 0, nullptr);
        if (fd == -1)
        {
            eio("failed to open input for reading");
            goto cleanup;
        }
        io->input.fd = fd;
    }
    else
        io->input.fd = STDIN_FILENO;

    if (output)
    {
        int fd =
            uv_fs_open(io->loop, &io->output.req, output, O_WRONLY, 0, nullptr);
        if (fd == -1)
        {
            eio("failed to open output for writing");
            goto cleanup;
        }
        io->output.fd = fd;
    }
    else
        io->output.fd = STDOUT_FILENO;

    return true;

cleanup:
    io_cleanup(io);
    return false;
}

void io_cleanup(struct io *io)
{
    if (io->input.fd != -1 || io->input.fd != STDIN_FILENO)
        uv_fs_close(io->loop, &io->input.req, io->input.fd, nullptr);

    if (io->output.fd != -1 || io->output.fd != STDOUT_FILENO)
        uv_fs_close(io->loop, &io->output.req, io->output.fd, nullptr);

    io->input.fd = -1;
    io->output.fd = -1;
}
