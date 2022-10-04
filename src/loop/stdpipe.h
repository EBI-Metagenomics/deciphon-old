#ifndef PIPE_STDPIPE_H
#define PIPE_STDPIPE_H

#include "uv.h"
#include <stdbool.h>
#include <unistd.h>

struct uv_loop_s;

struct stdpipe
{
    struct uv_pipe_s uvpipe;
    int fd;
};

void stdpipe_init(struct stdpipe *, struct uv_loop_s *);
void stdpipe_open(struct stdpipe *, int fd);
void stdpipe_close(struct stdpipe *);

#endif