#ifndef SCHED_COUNT_H
#define SCHED_COUNT_H

#include "deciphon/core/rc.h"

struct count
{
    unsigned count;
};

struct jr;

enum rc count_parse(struct count *count, struct jr *jr);

#endif
