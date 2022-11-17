#ifndef LOOP_STDPIPES_H
#define LOOP_STDPIPES_H

#include "loop/callbacks.h"

struct uv_pipe_s;

void stdpipes_init(on_exit_fn_t *);
struct uv_pipe_s *stdpipes_uv(int fd);
void stdpipes_close(void);

#endif
