#ifndef SEQUENCE_H
#define SEQUENCE_H

#include "lib/c-list.h"

struct sequence
{
    char const* sequence;
    CList       link;
};

#endif
