#include "dcp/srv.h"
#include "elapsed/elapsed.h"
#include "cco/cco.h"
#include "db_pool.h"
#include "dcp/db.h"
#include "dcp/job.h"
#include "dcp/pro_state.h"
#include "dcp/rc.h"
#include "error.h"
#include "macros.h"
#include "sched.h"
#include "sched_db.h"
#include "sched_job.h"
#include "table.h"
#include "work.h"
#include "xfile.h"

struct dcp_srv
{
    struct dcp_job job;
    struct dcp_prod prod;
};

struct dcp_srv *dcp_srv_open(char const *filepath)
{
    struct dcp_srv *srv = malloc(sizeof(*srv));
    if (!srv)
    {
        error(DCP_OUTOFMEM, "failed to malloc server");
        goto cleanup;
    }
    db_pool_module_init();

    enum dcp_rc rc = DCP_DONE;
    if ((rc = sched_setup(filepath))) goto cleanup;
    if ((rc = sched_open(filepath))) goto cleanup;

    return srv;

cleanup:
    free(srv);
    return NULL;
}

enum dcp_rc dcp_srv_close(struct dcp_srv *srv)
{
    enum dcp_rc rc = sched_close();
    free(srv);
    return rc;
}

enum dcp_rc dcp_srv_add_db(struct dcp_srv *srv, char const *name,
                           char const *filepath, int64_t *id)
{
    if (!xfile_is_readable(filepath))
        return error(DCP_IOERROR, "file is not readable");

    struct sched_db db = SCHED_DB_INIT();
    sched_db_setup(&db, name, filepath);

    enum dcp_rc rc = sched_db_add(&db);
    *id = db.id;
    return rc;
}

enum dcp_rc dcp_srv_submit_job(struct dcp_srv *srv, struct dcp_job *job)
{
    return sched_submit_job(job);
}

enum dcp_rc dcp_srv_job_state(struct dcp_srv *srv, int64_t job_id,
                              enum dcp_job_state *state)
{
    return sched_job_state(job_id, state);
}

enum dcp_rc dcp_srv_run(struct dcp_srv *srv, bool run_once)
{
    enum dcp_rc rc = DCP_DONE;
    struct work work = {0};
    work_init(&work);

    while (true)
    {
        if ((rc = work_next(&work)) == DCP_NOTFOUND)
        {
            if (run_once)
                break;
            elapsed_sleep(500);
            continue;
        }
        if (rc != DCP_NEXT) return rc;

        rc = work_run(&work);
        if (rc) return rc;
    }

    return DCP_DONE;
}

enum dcp_rc dcp_srv_next_prod(struct dcp_srv *srv, int64_t job_id,
                              int64_t *prod_id)
{
    enum dcp_rc rc = sched_prod_next(job_id, prod_id);
    if (rc == DCP_DONE) return rc;
    if (rc != DCP_NEXT) return rc;
    if ((rc = sched_prod_get(&srv->prod, *prod_id))) return rc;
    return DCP_NEXT;
}

struct dcp_prod const *dcp_srv_get_prod(struct dcp_srv const *srv)
{
    return &srv->prod;
}
