#ifndef DECIPHON_CORE_LOOPER_H
#define DECIPHON_CORE_LOOPER_H

#include "uv.h"
#include <stdbool.h>

struct looper
{
    bool terminating;
    struct uv_loop_s *loop;
    struct uv_async_s async;
    struct uv_signal_s sigterm;
    struct uv_signal_s sigint;

    struct
    {
        bool async;
        bool sigterm;
        bool sigint;
    } closing;
};

void looper_init(struct looper *);
void looper_run(struct looper *);
void looper_cleanup(struct looper *);

#endif
