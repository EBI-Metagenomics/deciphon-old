#ifndef DECY_SCHEDY_H
#define DECY_SCHEDY_H

#include "decy/callbacks.h"
#include <stdbool.h>

struct uv_loop_s;

void schedy_init(struct uv_loop_s *, onterm_fn_t *, void *arg);
void schedy_setup(char const *uri, char const *key, onreply_fn_t *);
void schedy_is_online(onreply_fn_t *);
void schedy_terminate(void);
bool schedy_isterminated(void);

#endif
