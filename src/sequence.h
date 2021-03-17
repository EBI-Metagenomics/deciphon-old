#ifndef SEQUENCE_H
#define SEQUENCE_H

#include "list.h"

struct sequence
{
    char const* sequence;
    struct list link;
};

#endif
