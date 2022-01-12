#include "job.h"
#include "common/compiler.h"
#include "common/rc.h"
#include "common/safe.h"
#include "common/utc.h"
#include "sched/job.h"
#include "sched/sched.h"
#include "seq.h"
#include "stmt.h"
#include "xsql.h"
#include <sqlite3.h>
#include <stdlib.h>
#include <string.h>

extern struct sqlite3 *sched;

void sched_job_init(struct sched_job *job, int64_t db_id, bool multi_hits,
                    bool hmmer3_compat)
{
    job->id = 0;
    job->db_id = db_id;
    job->multi_hits = multi_hits;
    job->hmmer3_compat = hmmer3_compat;
    safe_strcpy(job->state, "pend", ARRAY_SIZE_OF(*job, state));

    job->error[0] = 0;
    job->submission = 0;
    job->exec_started = 0;
    job->exec_ended = 0;
}

enum rc job_submit(struct sched_job *job)
{
    job->submission = utc_now();
    struct sqlite3_stmt *st = stmt[JOB_INSERT];
    if (xsql_reset(st)) return failed_to(EFAIL, "reset");

    if (xsql_bind_i64(st, 0, job->db_id)) return failed_to(EFAIL, "bind");
    if (xsql_bind_i64(st, 1, job->multi_hits))
        return failed_to(EFAIL, "bind");
    if (xsql_bind_i64(st, 2, job->hmmer3_compat))
        return failed_to(EFAIL, "bind");
    if (xsql_bind_str(st, 3, job->state)) return failed_to(EFAIL, "bind");

    if (xsql_bind_str(st, 4, job->error)) return failed_to(EFAIL, "bind");
    if (xsql_bind_i64(st, 5, job->submission))
        return failed_to(EFAIL, "bind");
    if (xsql_bind_i64(st, 6, job->exec_started))
        return failed_to(EFAIL, "bind");
    if (xsql_bind_i64(st, 7, job->exec_ended))
        return failed_to(EFAIL, "bind");

    if (xsql_step(st)) return failed_to(EFAIL, "step");
    job->id = xsql_last_id(sched);
    return DONE;
}

static int next_pending_job_id(int64_t *job_id)
{
    struct sqlite3_stmt *st = stmt[JOB_GET_PEND];
    if (xsql_reset(st)) return failed_to(EFAIL, "reset");

    int rc = xsql_step(st);
    if (rc == DONE) return NOTFOUND;
    if (rc != NEXT) return failed_to(EFAIL, "get pend job");
    *job_id = sqlite3_column_int64(st, 0);
    if (xsql_step(st)) return failed_to(EFAIL, "get pend job");

    st = stmt[JOB_SET_RUN];
    if (xsql_reset(st)) return failed_to(EFAIL, "reset");

    if (xsql_bind_i64(st, 0, utc_now())) return failed_to(EFAIL, "bind");
    if (xsql_bind_i64(st, 1, *job_id)) return failed_to(EFAIL, "bind");
    if (xsql_step(st)) return failed_to(EFAIL, "step");
    return DONE;
}

enum rc job_next_pending(struct sched_job *job)
{
    int rc = next_pending_job_id(&job->id);
    if (rc == NOTFOUND) return NOTFOUND;
    if (rc != DONE) return EFAIL;
    return job_get(job);
}

enum rc job_set_error(int64_t job_id, char const *error, int64_t exec_ended)
{
    struct sqlite3_stmt *st = stmt[JOB_SET_ERROR];
    if (xsql_reset(st)) return failed_to(EFAIL, "reset");

    if (xsql_bind_str(st, 0, error)) return failed_to(EFAIL, "bind");
    if (xsql_bind_i64(st, 1, exec_ended)) return failed_to(EFAIL, "bind");
    if (xsql_bind_i64(st, 2, job_id)) return failed_to(EFAIL, "bind");

    if (xsql_step(st)) return failed_to(EFAIL, "step");
    return DONE;
}

enum rc job_set_done(int64_t job_id, int64_t exec_ended)
{
    struct sqlite3_stmt *st = stmt[JOB_SET_DONE];
    if (xsql_reset(st)) return failed_to(EFAIL, "reset");

    if (xsql_bind_i64(st, 0, exec_ended)) return failed_to(EFAIL, "bind");
    if (xsql_bind_i64(st, 1, job_id)) return failed_to(EFAIL, "bind");

    if (xsql_step(st)) return failed_to(EFAIL, "step");
    return DONE;
}

static enum sched_job_state resolve_job_state(char const *state)
{
    if (strcmp("pend", state) == 0)
        return SCHED_JOB_PEND;
    else if (strcmp("run", state) == 0)
        return SCHED_JOB_RUN;
    else if (strcmp("done", state) == 0)
        return SCHED_JOB_DONE;
    else if (strcmp("fail", state) == 0)
        return SCHED_JOB_FAIL;

    BUG();
}

enum rc sched_job_state(int64_t job_id, enum sched_job_state *state)
{
    struct sqlite3_stmt *st = stmt[JOB_GET_STATE];
    if (xsql_reset(st)) return failed_to(EFAIL, "reset");

    if (xsql_bind_i64(st, 0, job_id)) return failed_to(EFAIL, "bind");

    int rc = xsql_step(st);
    if (rc == DONE) return NOTFOUND;
    if (rc != NEXT) return failed_to(EFAIL, "get job state");

    char tmp[SCHED_JOB_STATE_SIZE] = {0};
    rc = xsql_cpy_txt(st, 0, (struct xsql_txt){SCHED_JOB_STATE_SIZE, tmp});
    if (rc) return EFAIL;
    *state = resolve_job_state(tmp);

    if (xsql_step(st)) return failed_to(EFAIL, "step");
    return DONE;
}

enum rc job_get(struct sched_job *job)
{
    struct sqlite3_stmt *st = stmt[JOB_SELECT];
    if (xsql_reset(st)) return failed_to(EFAIL, "reset");

    if (xsql_bind_i64(st, 0, job->id)) return failed_to(EFAIL, "bind");

    if (xsql_step(st) != NEXT) return EFAIL;

    job->id = sqlite3_column_int64(st, 0);

    job->db_id = sqlite3_column_int64(st, 1);
    job->multi_hits = sqlite3_column_int(st, 2);
    job->hmmer3_compat = sqlite3_column_int(st, 3);
    if (xsql_cpy_txt(st, 4, XSQL_TXT_OF(*job, state))) return EFAIL;

    if (xsql_cpy_txt(st, 5, XSQL_TXT_OF(*job, error))) return EFAIL;
    job->submission = sqlite3_column_int64(st, 6);
    job->exec_started = sqlite3_column_int64(st, 7);
    job->exec_ended = sqlite3_column_int64(st, 8);

    if (xsql_step(st)) return failed_to(EFAIL, "step");
    return DONE;
}
