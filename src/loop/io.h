#ifndef LOOP_IO_H
#define LOOP_IO_H

#include "core/c23.h"
#include "uv.h"
#include <stdbool.h>

struct io
{
    struct uv_loop_s *loop;
    struct
    {
        int fd;
        struct uv_fs_s req;
    } input;
    struct
    {
        int fd;
        struct uv_fs_s req;
    } output;
};

#define IO_DECLARE(NAME) struct io NAME = {nullptr, {-1, {0}}, {-1, {0}}}

void io_init(struct io *, struct uv_loop_s *);
bool io_setup(struct io *, char const *input, char const *output);
void io_cleanup(struct io *);

#endif
