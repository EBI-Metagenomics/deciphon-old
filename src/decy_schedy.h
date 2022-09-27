#ifndef DECY_SCHEDY_H
#define DECY_SCHEDY_H

struct uv_loop_s;

typedef void decy_schedy_onterm_fn_t(void *arg);

void decy_schedy_init(struct uv_loop_s *, decy_schedy_onterm_fn_t *, void *arg);
void decy_schedy_cleanup(void);

#endif
