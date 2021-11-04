#ifndef SCHED_DB_H
#define SCHED_DB_H

#include "path.h"
#include "xstrlcpy.h"
#include <stdint.h>

struct sched_db
{
    uint64_t id;
    char filepath[PATH_SIZE];
};

static inline void sched_abc_init(struct sched_db *db, char filepath[PATH_SIZE])
{
    db->id = 0;
    xstrlcpy(db->filepath, filepath, PATH_SIZE);
}

#endif
