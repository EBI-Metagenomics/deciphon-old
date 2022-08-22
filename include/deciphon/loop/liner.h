#ifndef DECIPHON_LOOP_LINER_H
#define DECIPHON_LOOP_LINER_H

#include "uv.h"
#include <stdbool.h>

struct liner;
struct looper;

typedef void liner_ioerror_fn_t(void);
typedef void liner_newline_fn_t(char *line);

struct liner *liner_new(struct looper *, liner_ioerror_fn_t *,
                        liner_newline_fn_t *);
void liner_open(struct liner *, uv_file fd);
void liner_write(struct liner *, unsigned size, char *line);
void liner_del(struct liner *);

#endif
