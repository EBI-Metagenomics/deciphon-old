#ifndef SCANNY_SCANNY_H
#define SCANNY_SCANNY_H

#include "loop/looper.h"
#include "loop/loopio.h"

struct scanny
{
    struct looper looper;
    struct loopio loopio;
};

extern struct scanny scanny;

#endif
