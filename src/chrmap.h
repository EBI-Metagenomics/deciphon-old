#ifndef CHRMAP_H
#define CHRMAP_H

#include "dcp/rc.h"

#define CHRMAP_SIZE 32

struct chrmap
{
    char map[CHRMAP_SIZE];
    unsigned bits;
    unsigned long chars;
    unsigned long *data;
};

void chrmap_init(struct chrmap *x, char map[CHRMAP_SIZE]);

char chrmap_get(struct chrmap const *x, unsigned long pos);

struct chrmap *chrmap_realloc(struct chrmap *x, unsigned long chars);

void chrmap_set(struct chrmap *x, char val, unsigned long pos);

void chrmap_del(struct chrmap *x);

#endif
