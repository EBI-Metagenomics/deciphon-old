#ifndef CORE_THRFILES_H
#define CORE_THRFILES_H

#include "core/limits.h"
#include <stdio.h>

struct thrfiles_final
{
    FILE *file;
    char path[FILENAME_MAX];
};

struct thrfiles
{
    int size;
    FILE *files[NUM_THREADS];
    struct thrfiles_final final;
};

void thrfiles_init(struct thrfiles *);
int thrfiles_setup(struct thrfiles *, int nthreads);
int thrfiles_finishup(struct thrfiles *, char const **);
void thrfiles_cleanup(struct thrfiles *);

#endif
