#ifndef DCP_JOBS_H
#define DCP_JOBS_H

#include <sqlite3.h>

struct dcp_jobs
{
    sqlite3 *db;
};

#define DCP_JOBS_INIT()                                                        \
    (struct dcp_jobs) { .db = NULL }

#endif
