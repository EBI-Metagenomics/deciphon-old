#include "work.h"
#include "db_pool.h"
#include "dcp.h"
#include "dcp/generics.h"
#include "error.h"
#include "sched.h"
#include "xfile.h"
#include "xstrlcpy.h"

static enum dcp_rc open_work(struct work *work);
static enum dcp_rc close_work(struct work *work);
static enum dcp_rc next_profile(struct work *work);

enum dcp_rc work_fetch(struct work *work, struct sched *sched,
                       struct db_pool *pool)
{
    enum dcp_rc rc = sched_next_job(sched, &work->job);
    if (rc == DCP_DONE) return DCP_DONE;
    work->db = db_pool_fetch(pool, work->job.db_id);
    if (!work->db) return error(DCP_FAIL, "reached limit of open db handles");

    work->db_path[0] = 0;
    if ((rc = sched_db_filepath(sched, work->job.db_id, work->db_path)))
        return rc;
    return DCP_NEXT;
}

enum dcp_rc work_run(struct work *work)
{
    enum dcp_rc rc = open_work(work);
    if (rc) return rc;

    while ((rc = next_profile(work)) == DCP_NEXT)
    {
            /* struct imm_seq s = imm_seq(imm_str(data), abc); */
            /* if (imm_task_setup(work->task, &s)) */
            /*     return error(DCP_FAIL, "failed to setup task"); */
        /* work->abc; */
        /* work->prof; */
    }

    return close_work(work);

cleanup:
    close_work(work);
    return rc;
}

static enum dcp_rc open_work(struct work *work)
{
    work->db->fd = NULL;
    enum dcp_rc rc = prod_file_open(&work->prod_file);
    if (rc) goto cleanup;

    work->db->fd = fopen(work->db_path, "rb");
    if (!work->db->fd)
    {
        rc = error(DCP_IOERROR, "failed to open db");
        goto cleanup;
    }

    if ((rc = dcp_pro_db_openr(&work->db->pro, work->db->fd))) goto cleanup;

    return DCP_DONE;

cleanup:
    close_work(work);
    return rc;
}

static enum dcp_rc close_work(struct work *work)
{
    enum dcp_rc rc = dcp_pro_db_close(&work->db->pro);
    if (rc) return rc;
    if (work->db->fd && fclose(work->db->fd))
        return error(DCP_IOERROR, "failed to close file");
    return prod_file_close(&work->prod_file);
}

static enum dcp_rc next_profile(struct work *work)
{
    if (dcp_db_end(dcp_super(&work->db->pro))) return DCP_DONE;

    struct dcp_pro_prof *prof = dcp_pro_db_profile(&work->db->pro);

    enum dcp_rc rc = dcp_pro_db_read(&work->db->pro, prof);
    if (rc) return rc;

    work->prof = prof;
    work->abc = work->prof->super.abc;

    if (!work->task && !(work->task = imm_task_new(&work->prof->alt.dp)))
        return error(DCP_FAIL, "failed to create task");

    if (imm_task_reset(work->task, &work->prof->alt.dp))
        return error(DCP_FAIL, "failed to reset task");

    return DCP_NEXT;
}
