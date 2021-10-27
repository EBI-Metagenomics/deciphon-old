#ifndef SCHED_DB_H
#define SCHED_DB_H

#include "dcp/strlcpy.h"
#include "filepath.h"
#include <stdint.h>

struct sched_db
{
    uint64_t id;
    char filepath[FILEPATH_SIZE];
};

static inline void sched_abc_init(struct sched_db *db,
                                  char filepath[FILEPATH_SIZE])
{
    db->id = 0;
    dcp_strlcpy(db->filepath, filepath, FILEPATH_SIZE);
}

#endif
