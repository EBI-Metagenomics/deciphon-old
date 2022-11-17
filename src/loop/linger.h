#ifndef LOOP_LINGER_H
#define LOOP_LINGER_H

#include <stdbool.h>

struct uv_loop_s;
typedef bool on_linger_fn_t(void);
typedef void on_cleanup_fn_t(void);

void linger_init(struct uv_loop_s *, on_linger_fn_t *, on_cleanup_fn_t *);
void linger_trigger(void);
void linger_cleanup(void);

#endif
