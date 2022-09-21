#ifndef SCANNY_H
#define SCANNY_H

#include "core/limits.h"
#include "loop/looper.h"
#include "loop/loopio.h"

struct scanny
{
    struct looper looper;
    struct loopio loopio;
};

extern struct scanny scanny;

#endif
