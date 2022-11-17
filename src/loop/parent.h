#ifndef LOOP_PARENT_H
#define LOOP_PARENT_H

#include "loop/callbacks.h"
#include <stdbool.h>

void parent_init(on_read2_fn_t *, on_eof2_fn_t *, on_error2_fn_t *);
void parent_send(char const *string);
void parent_close(void);
bool parent_closed(void);

#endif
