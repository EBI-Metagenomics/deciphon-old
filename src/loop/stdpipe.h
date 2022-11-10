#ifndef LOOP_STDPIPE_H
#define LOOP_STDPIPE_H

#include "loop/callbacks.h"
#include "uv.h"

struct stdpipe
{
    struct uv_pipe_s uvpipe;
    int fd;
    on_exit2_fn_t *on_exit;
    void *arg;
    bool no_close;
};

void stdpipe_init(struct stdpipe *, struct uv_loop_s *, on_exit2_fn_t *,
                  void *);
void stdpipe_open(struct stdpipe *, int fd);
void stdpipe_close(struct stdpipe *);

#endif
