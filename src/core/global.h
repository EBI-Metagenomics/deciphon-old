#ifndef PIPE_GLOBAL_H
#define PIPE_GLOBAL_H

#include "callbacks.h"

struct uv_loop_s;

void global_init(on_term_fn_t *, int argc, char *argv[]);
char const *global_title(void);
struct uv_loop_s *global_loop(void);
void global_terminate(void);
void global_run(void);
void global_cleanup(void);

#endif
