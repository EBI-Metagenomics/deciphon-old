#include "work.h"
#include "db_pool.h"
#include "dcp.h"
#include "error.h"
#include "sched.h"
#include "xfile.h"
#include "xstrlcpy.h"
#include "dcp/generics.h"

static enum dcp_rc open_work(struct work *work);

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

    while (!(rc = dcp_db_end(dcp_super(&work->db->pro))))
    {
        struct dcp_pro_prof *prof = dcp_pro_db_profile(&work->db->pro);
        if ((rc = dcp_pro_db_read(&work->db->pro, prof))) goto cleanup;
        struct imm_abc const *abc = prof->super.abc;

    }
cleanup:
    return DCP_DONE;
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

    if ((rc = dcp_pro_db_openr(&work->db->pro, work->db->fd)))
        goto cleanup;

    return DCP_DONE;

cleanup:
    dcp_pro_db_close(&work->db->pro);
    fclose(work->db->fd);
    prod_file_close(&work->prod_file);
    return rc;
}
