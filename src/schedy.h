#ifndef SCHEDY_H
#define SCHEDY_H

#include "loop/looper.h"
#include "loop/loopio.h"

struct schedy
{
    struct looper looper;
    struct loopio loopio;
};

#endif
