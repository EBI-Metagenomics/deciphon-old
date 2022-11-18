#ifndef MULTIFILE_H
#define MULTIFILE_H

#include "deciphon_limits.h"
#include <stdio.h>

struct multifile_final
{
    FILE *file;
    char path[FILENAME_MAX];
};

struct multifile
{
    int size;
    FILE *files[NUM_THREADS];
    struct multifile_final final;
};

void multifile_init(struct multifile *);
int multifile_setup(struct multifile *, int nthreads);
int multifile_finishup(struct multifile *, char const **);
void multifile_cleanup(struct multifile *);

#endif
