#ifndef DB_HANDLE_H
#define DB_HANDLE_H

#include "cco/cco.h"
#include "dcp_limits.h"
#include "path.h"
#include "protein_db.h"
#include "rc.h"
#include "standard_db.h"
#include <stdio.h>

#define DB_HANDLE_MAX_FILES DCP_DB_HANDLE_MAX_FILES

struct db_handle
{
    int64_t id;
    unsigned pool_id;
    struct cco_hnode hnode;
    int nfiles;
    FILE *fp[DB_HANDLE_MAX_FILES];
    uint64_t open_since;
    union
    {
        struct dcp_standard_db std;
        struct dcp_protein_db pro;
    };
};

void db_handle_init(struct db_handle *db, int64_t id);
enum rc db_handle_open(struct db_handle *db, char path[DCP_PATH_SIZE],
                           unsigned nfiles);
enum rc db_handle_close(struct db_handle *db);

#endif
