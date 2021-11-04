#include "work.h"
#include "db_pool.h"
#include "dcp.h"
#include "error.h"
#include "sched.h"
#include "xfile.h"
#include "xstrlcpy.h"

static enum dcp_rc fetch_db(struct db_pool *pool, int64_t id,
                            struct db_handle **db);

enum dcp_rc work_fetch(struct work *work, struct sched *sched,
                       struct db_pool *pool)
{
    enum dcp_rc rc = sched_next_job(sched, &work->job);
    if (rc == DCP_DONE) return DCP_DONE;
    return fetch_db(pool, work->job.db_id, &work->db);
}

enum dcp_rc work_run(struct work *work) { return DCP_DONE; }

static enum dcp_rc prod_file_open(struct prod_file *file)
{
    xstrlcpy(file->path, PATH_TEMP_TEMPLATE, MEMBER_SIZE(*file, path));
    enum dcp_rc rc = xfile_mktemp(file->path);
    if (rc) return rc;
    if (!(file->fd = fopen(file->path, "wb")))
        rc = error(DCP_IOERROR, "failed to open prod file");
    return rc;
}

static enum dcp_rc fetch_db(struct db_pool *pool, int64_t id,
                            struct db_handle **db)
{
    struct db_handle *tmp = db_pool_get(pool, id);
    if (!tmp && !(tmp = db_pool_new(pool, id)))
        return error(DCP_FAIL, "reached limit of open db handles");
    *db = tmp;
    return DCP_DONE;
}
