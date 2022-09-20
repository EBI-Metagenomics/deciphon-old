#ifndef LOOP_IO_H
#define LOOP_IO_H

#include "core/c23.h"
#include "uv.h"
#include <stdbool.h>

typedef void io_onopen_fn_t(bool ok, void *arg);
typedef void io_onclose_fn_t(void *arg);

struct io
{
    struct uv_loop_s *loop;
    io_onopen_fn_t *onopen_cb;
    io_onclose_fn_t *onclose_cb;
    void *arg;
    int fd;
    struct uv_fs_s req;
};

void io_init(struct io *, struct uv_loop_s *, io_onopen_fn_t *,
             io_onclose_fn_t *, void *arg);
void io_open(struct io *, char const *file, int flags, int mode);
static inline int io_fd(struct io const *io) { return io->fd; }
void io_close(struct io *);

#endif
