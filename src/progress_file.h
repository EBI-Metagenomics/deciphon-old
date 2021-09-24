#ifndef PROGRESS_FILE_H
#define PROGRESS_FILE_H

#include "athr/athr.h"
#include <stdio.h>

struct progress_file
{
    bool enabled;
    struct athr at;
    long pos;
    FILE *fd;
};

void progress_file_init(struct progress_file *p, FILE *fd);
void progress_file_start(struct progress_file *p, bool enabled);
void progress_file_update(struct progress_file *p);
void progress_file_stop(struct progress_file *p);

#endif
