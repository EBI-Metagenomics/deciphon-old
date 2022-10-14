#ifndef CORE_GLOBAL_H
#define CORE_GLOBAL_H

#include "loop/callbacks.h"

struct uv_loop_s;

void global_init(on_term_fn_t *, int argc, char *argv[], int log_level);
char const *global_title(void);
struct uv_loop_s *global_loop(void);
void global_terminate(void);
void global_run(void);
void global_cleanup(void);

#endif
