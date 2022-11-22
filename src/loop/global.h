#ifndef LOOP_GLOBAL_H
#define LOOP_GLOBAL_H

#include <stdbool.h>

struct uv_loop_s;
typedef bool on_linger_fn_t(void);
typedef void on_cleanup_fn_t(void);

void global_init(char *const, int loglvl);
void global_linger_setup(on_linger_fn_t *, on_cleanup_fn_t *);
struct uv_loop_s *global_loop(void);
void global_shutdown(void);
int global_run(void);
int global_run_once(void);

#endif
