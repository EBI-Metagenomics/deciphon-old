#ifndef PRESSY_H
#define PRESSY_H

#include "core/limits.h"
#include "loop/looper.h"
#include "loop/loopio.h"

struct pressy
{
    struct looper looper;
    struct loopio loopio;
};

extern struct pressy pressy;

#endif
