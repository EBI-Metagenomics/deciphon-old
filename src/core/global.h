#ifndef CORE_GLOBAL_H
#define CORE_GLOBAL_H

#include "loop/callbacks.h"

struct uv_loop_s;

void global_init(on_term_fn_t *, char *const arg0, int log_level);
char const *global_title(void);
char const *global_exepath(void);
char const *global_exedir(void);
long global_now(void);
struct uv_loop_s *global_loop(void);
void global_terminate(void);
void global_run(void);
int global_cleanup(void);

#endif
