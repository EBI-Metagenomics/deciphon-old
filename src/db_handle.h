#ifndef DB_HANDLE_H
#define DB_HANDLE_H

#include "cco/cco.h"
#include "dcp/pro_db.h"
#include "dcp/std_db.h"
#include <stdio.h>

struct db_handle
{
    int64_t sched_id;
    unsigned pool_id;
    struct cco_hnode hnode;
    FILE *fd;
    union
    {
        struct dcp_std_db std;
        struct dcp_pro_db pro;
    };
};

#endif
