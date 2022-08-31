#ifndef SCHED_COUNT_H
#define SCHED_COUNT_H

#include "deciphon/core/rc.h"

struct count
{
    unsigned count;
};

struct xjson;

enum rc count_parse(struct count *count, struct xjson *x, unsigned start);

#endif
