#ifndef HASHFILE_H
#define HASHFILE_H

#include "deciphon_limits.h"
#include <stdio.h>

struct hashfile
{
    char dirpath[PATH_SIZE];
    char filepath[PATH_SIZE];
    FILE *file;
    long xxh3;
};

int hashfile_init(struct hashfile *, char const *dir);
int hashfile_open(struct hashfile *);
int hashfile_write(struct hashfile *, char const *fmt, ...)
    __attribute__((format(printf, 2, 3)));
int hashfile_close(struct hashfile *);
long hashfile_xxh3(struct hashfile const *);

#endif
