#ifndef CORE_GLOBAL_H
#define CORE_GLOBAL_H

#include "loop/callbacks.h"

struct uv_loop_s;

void global_init(on_term_fn_t *, on_linger_fn_t *, on_exit_fn_t *, char *const);
void global_setlog(int log_level);
char const *global_title(void);
char const *global_exepath(void);
char const *global_exedir(void);
long global_now(void);
struct uv_loop_s *global_loop(void);
void global_terminate(void);
_Noreturn void global_die(void);
int global_run(void);

#endif
