#include "dcp/server.h"
#include "dcp/std_db.h"
#include "dcp/db.h"
#include "dcp/job.h"
#include "dcp/prof_types.h"
#include "dcp_file.h"
#include "error.h"
#include "filepath.h"
#include "sched.h"

struct dcp_server
{
    struct sched sched;
};

struct dcp_server *dcp_server_open(char const *filepath)
{
    struct dcp_server *srv = malloc(sizeof(*srv));
    if (!srv)
    {
        error(DCP_OUTOFMEM, "failed to malloc server");
        goto cleanup;
    }

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
    FILE *fd = NULL;
    struct dcp_std_db db;
    dcp_std_db_init(&db);

    struct dcp_job job;
    char db_fp[FILEPATH_SIZE] = {0};
    enum dcp_rc rc = sched_next_pend_job(&srv->sched, &job, db_fp);
    if (rc == DCP_DONE) return DCP_DONE;

    if (!(fd = fopen(db_fp, "rb")))
        return error(DCP_IOERROR, "failed to open db file");

    if ((rc = dcp_std_db_openr(&db, fd)))
        goto cleanup;

    if ((rc = dcp_std_db_close(&db)))
        goto cleanup;

    fclose(fd);
    return DCP_NEXT;

cleanup:
    dcp_std_db_close(&db);
    fclose(fd);
    return rc;
}
