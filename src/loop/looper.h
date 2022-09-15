#ifndef DECIPHON_LOOP_LOOPER_H
#define DECIPHON_LOOP_LOOPER_H

#include "uv.h"
#include <stdbool.h>

struct looper;

typedef void looper_onterm_fn_t(void);

struct looper
{
    bool terminating;
    struct uv_loop_s *loop;
    struct uv_async_s async;
    struct uv_signal_s sigterm;
    struct uv_signal_s sigint;

    looper_onterm_fn_t *onterm_cb;

    struct
    {
        bool async;
        bool sigterm;
        bool sigint;
    } closing;

    struct
    {
        bool async;
        bool sigterm;
        bool sigint;
    } closed;
};

void looper_init(struct looper *, looper_onterm_fn_t *);
void looper_run(struct looper *);
void looper_terminate(struct looper *);
void looper_cleanup(struct looper *);

#endif
