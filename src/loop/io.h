#ifndef LOOP_IO_H
#define LOOP_IO_H

#include "core/c23.h"
#include "uv.h"
#include <stdbool.h>

typedef void io_onopen_fn_t(bool ok);
typedef void io_onclose_fn_t(void);

struct io
{
    struct uv_loop_s *loop;
    io_onopen_fn_t *onopen_cb;
    io_onclose_fn_t *onclose_cb;
    struct
    {
        int fd;
        struct uv_fs_s req;
        bool is_open;
        bool is_over;
    } input;
    struct
    {
        int fd;
        struct uv_fs_s req;
        bool is_open;
        bool is_over;
    } output;
};

void io_init(struct io *, struct uv_loop_s *, io_onopen_fn_t *,
             io_onclose_fn_t *);
void io_setup(struct io *, char const *input, char const *output);
void io_close(struct io *);

#endif
