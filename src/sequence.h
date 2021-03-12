#ifndef SEQUENCE_H
#define SEQUENCE_H

#include "lib/list.h"

struct sequence
{
    char const* sequence;
    struct list link;
};

#endif
