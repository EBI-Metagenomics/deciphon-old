#ifndef DECY_DECY_H
#define DECY_DECY_H

#include "core/limits.h"
#include "loop/looper.h"
#include "loop/loopio.h"

struct decy
{
    struct looper looper;
};

extern struct decy decy;

#endif
