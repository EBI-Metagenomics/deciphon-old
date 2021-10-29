#include "dcp/server.h"
#include "dcp/pro_db.h"
#include "cco/cco.h"
#include "db_tbl.h"
#include "dcp/db.h"
#include "dcp/generics.h"
#include "dcp/job.h"
#include "dcp/prof_types.h"
#include "dcp/pro_db.h"
#include "dcp_file.h"
#include "error.h"
#include "filepath.h"
#include "sched.h"

struct dcp_server
{
    struct sched sched;
    struct db_tbl db_tbl;
};

struct dcp_server *dcp_server_open(char const *filepath)
{
    struct dcp_server *srv = malloc(sizeof(*srv));
    if (!srv)
    {
        error(DCP_OUTOFMEM, "failed to malloc server");
        goto cleanup;
    }
    db_tbl_init(&srv->db_tbl);

    enum dcp_rc rc = DCP_DONE;
    if ((rc = sched_setup(filepath))) goto cleanup;
    if ((rc = sched_open(&srv->sched, filepath))) goto cleanup;

    return srv;

cleanup:
    free(srv);
    return NULL;
}

enum dcp_rc dcp_server_close(struct dcp_server *srv)
{
    enum dcp_rc rc = sched_close(&srv->sched);
    free(srv);
    return rc;
}

enum dcp_rc dcp_server_add_db(struct dcp_server *srv, char const *filepath,
                              uint64_t *id)
{
    if (!file_readable(filepath))
        return error(DCP_IOERROR, "file is not readable");
    return sched_add_db(&srv->sched, filepath, id);
}

enum dcp_rc dcp_server_submit_job(struct dcp_server *srv, struct dcp_job *job,
                                  uint64_t db_id, uint64_t *job_id)
{
    return sched_submit_job(&srv->sched, job, db_id, job_id);
}

enum dcp_rc dcp_server_job_state(struct dcp_server *srv, uint64_t job_id,
                                 enum dcp_job_state *state)
{
    return sched_job_state(&srv->sched, job_id, state);
}

enum dcp_rc dcp_server_run(struct dcp_server *srv, bool blocking)
{
    struct dcp_job job;
    uint64_t job_id = 0;
    uint64_t db_id = 0;
    enum dcp_rc rc = sched_next_pend_job(&srv->sched, &job, &job_id, &db_id);
    if (rc == DCP_DONE) return DCP_DONE;

    struct db *db = db_tbl_get(&srv->db_tbl, db_id);
    if (!db)
    {
        if (!(db = db_tbl_new(&srv->db_tbl, db_id)))
        {
            rc = error(DCP_FAIL, "reached limit of open dbs");
            goto cleanup;
        }
    }

    char filepath[FILEPATH_SIZE] = {0};
    if ((rc = sched_db_filepath(&srv->sched, db_id, filepath))) goto cleanup;

    if (!(db->fd = fopen(filepath, "rb")))
        return error(DCP_IOERROR, "failed to open db file");

    if ((rc = dcp_pro_db_openr(&db->pro, db->fd))) goto cleanup;

    struct imm_result result = imm_result();
    struct imm_result null = imm_result();
    while (!(rc = dcp_db_end(dcp_super(&db->pro))))
    {
        struct dcp_pro_prof *prof = dcp_pro_db_profile(&db->pro);
        if ((rc = dcp_pro_db_read(&db->pro, prof))) goto cleanup;
        struct imm_abc const *abc = prof->super.abc;
        struct imm_task *task = imm_task_new(&prof->alt.dp);
        if (!task) return error(DCP_FAIL, "failed to create task");

        uint64_t seq_id = 0;
        char data[5001] = {0};
        while ((rc = sched_next_seq(&srv->sched, job_id, &seq_id, data) ==
                     DCP_NEXT))
        {
            struct imm_seq seq = imm_seq(imm_str(data), abc);

            if (imm_task_setup(task, &seq))
                return error(DCP_FAIL, "failed to create task");

            if (imm_dp_viterbi(&prof->alt.dp, task, &result))
                return error(DCP_FAIL, "failed to run viterbi");

            if (imm_dp_viterbi(&prof->null.dp, task, &null))
                return error(DCP_FAIL, "failed to run viterbi");

            imm_float lrt = -2 * (null.loglik - result.loglik);
            if (lrt < 100.0f) continue;
        }
    }
    if (rc != DCP_END) goto cleanup;

    dcp_pro_db_close(&db->pro);

    return DCP_NEXT;

cleanup:
    dcp_pro_db_close(&db->pro);
    fclose(db->fd);
    db_tbl_del(&srv->db_tbl, &db->hnode);
    return rc;
}
