#ifndef DB_HANDLE_H
#define DB_HANDLE_H

#include "dcp/pro_db.h"
#include "dcp/sched.h"
#include "cco/cco.h"
#include "dcp/std_db.h"
#include <stdio.h>

struct db_handle
{
    dcp_sched_id id;
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
