#include "dcp/srv.h"
#include "cco/cco.h"
#include "db_pool.h"
#include "dcp/db.h"
#include "dcp/job.h"
#include "dcp/pro_state.h"
#include "dcp/rc.h"
#include "elapsed/elapsed.h"
#include "error.h"
#include "macros.h"
#include "sched.h"
#include "sched_db.h"
#include "sched_job.h"
#include "table.h"
#include "work.h"
#include "xfile.h"
#include <signal.h>

static struct dcp_srv
{
    volatile atomic_bool interrupt;
    struct dcp_job job;
    struct dcp_prod prod;
} srv;

static void signal_handler(int sig)
{
    if (sig == SIGINT) atomic_store(&srv.interrupt, true);
}

enum dcp_rc dcp_srv_open(char const *filepath)
{
    atomic_store(&srv.interrupt, false);
    signal(SIGINT, signal_handler);
    db_pool_module_init();

    enum dcp_rc rc = DCP_DONE;
    if ((rc = sched_setup(filepath))) return rc;
    rc = sched_open(filepath);
    return rc;
}

enum dcp_rc dcp_srv_close(void) { return sched_close(); }

enum dcp_rc dcp_srv_add_db(char const *name, char const *filepath, int64_t *id)
{
    if (!xfile_is_readable(filepath))
        return error(DCP_IOERROR, "file is not readable");

    struct sched_db db = {0};
    enum dcp_rc rc = sched_db_setup(&db, name, filepath);
    if (rc) return rc;

    struct sched_db db2 = {0};
    rc = sched_db_get_by_xxh64(&db2, db.xxh64);
    if (rc == DCP_NOTFOUND)
    {
        rc = sched_db_add(&db);
        *id = db.id;
        return rc;
    }
    *id = db2.id;
    return rc;
}

enum dcp_rc dcp_srv_submit_job(struct dcp_job *job)
{
    return sched_submit_job(job);
}

enum dcp_rc dcp_srv_job_state(int64_t job_id, enum dcp_job_state *state)
{
    return sched_job_state(job_id, state);
}

enum dcp_rc dcp_srv_run(bool single_run)
{
    enum dcp_rc rc = DCP_DONE;
    struct work work = {0};
    work_init(&work);

    printf("Looking for work...\n");
    while (!atomic_load(&srv.interrupt))
    {
        if ((rc = work_next(&work)) == DCP_NOTFOUND)
        {
            if (single_run) break;
            elapsed_sleep(500);
            continue;
        }
        if (rc != DCP_NEXT) return rc;

        printf("Found work (%lld)...\n", work.job.id);
        rc = work_run(&work);
        printf("Finished work (%lld)!\n", work.job.id);
        if (rc) return rc;
    }

    return DCP_DONE;
}

enum dcp_rc dcp_srv_next_prod(int64_t job_id, int64_t *prod_id)
{
    enum dcp_rc rc = sched_prod_next(job_id, prod_id);
    if (rc == DCP_DONE) return rc;
    if (rc != DCP_NEXT) return rc;
    if ((rc = sched_prod_get(&srv.prod, *prod_id))) return rc;
    return DCP_NEXT;
}

struct dcp_prod const *dcp_srv_get_prod(void) { return &srv.prod; }
