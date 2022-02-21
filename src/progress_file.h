#ifndef DCP_PROGRESS_FILE_H
#define DCP_PROGRESS_FILE_H

#include "athr/athr.h"
#include <assert.h>
#include <stdio.h>

static_assert(sizeof(off_t) >= 8, "8 bytes for off_t");

struct progress_file
{
    bool enabled;
    struct athr at;
    off_t pos;
    FILE *fd;
};

void progress_file_init(struct progress_file *p, FILE *fd);
void progress_file_start(struct progress_file *p, bool enabled);
void progress_file_update(struct progress_file *p);
void progress_file_stop(struct progress_file *p);

#endif
