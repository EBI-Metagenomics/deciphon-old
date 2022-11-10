#ifndef LOOP_FIO_H
#define LOOP_FIO_H

#include "uv.h"
#include <stdbool.h>

typedef void fio_onopen_fn_t(bool ok, void *arg);
typedef void fio_onclose_fn_t(void *arg);

struct fio
{
    struct uv_loop_s *loop;
    fio_onopen_fn_t *onopen_cb;
    fio_onclose_fn_t *onclose_cb;
    void *arg;
    int fd;
    bool nofs_close;
    bool closed;
    struct uv_fs_s req;
};

void fio_init(struct fio *, struct uv_loop_s *, fio_onopen_fn_t *,
              fio_onclose_fn_t *, void *arg);
void fio_open(struct fio *, char const *file, int flags, int mode);
void fio_close(struct fio *);
int fio_fd(struct fio const *);
bool fio_isclosed(struct fio const *);

#endif
