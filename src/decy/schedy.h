#ifndef DECY_SCHEDY_H
#define DECY_SCHEDY_H

#include "decy/reply.h"
#include <stdbool.h>

struct uv_loop_s;

typedef void decy_schedy_onterm_fn_t(void *arg);

void decy_schedy_init(struct uv_loop_s *, decy_schedy_onterm_fn_t *, void *arg);
void decy_schedy_connect(char const *msg, decy_onreply_fn_t *);
void decy_schedy_is_online(decy_onreply_fn_t *);
void decy_schedy_terminate(void);
bool decy_schedy_isterminated(void);

#endif
