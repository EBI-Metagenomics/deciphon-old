#ifndef PRESSY_PRESSY_H
#define PRESSY_PRESSY_H

#include "loop/looper.h"
#include "loop/loopio.h"

struct pressy
{
    struct looper looper;
    struct loopio loopio;
};

extern struct pressy pressy;

#endif
