#ifndef LOOP_WHILE_H
#define LOOP_WHILE_H

#include "loop/now.h"
#include "loop/sleep.h"
#include "unused.h"

#define loop_while(msecs, cond)                                                \
    ({                                                                         \
        long deadline = now() + msecs;                                         \
        while (now() < deadline && (cond))                                     \
            sleep(500);                                                        \
    })

#endif
