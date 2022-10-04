#ifndef DECY_DECY_H
#define DECY_DECY_H

enum target
{
    TARGET_DECY,
    TARGET_PRESSY,
};

#include "loop/input.h"
#include "loop/output.h"

extern struct input input;
extern struct output output;
extern enum target target;

#endif
