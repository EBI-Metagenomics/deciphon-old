#include "job.h"
#include "common/rc.h"
#include "common/compiler.h"
#include "sched/job.h"
#include "sched/sched.h"
#include "common/safe.h"
#include "sched.h"
#include "seq.h"
#include "common/utc.h"
#include "xsql.h"
#include <sqlite3.h>
#include <stdlib.h>
#include <string.h>

extern struct sqlite3 *sched;

enum
{
    INSERT,
    GET_PEND,
    SET_RUN,
    GET_STATE,
    SELECT,
    SET_ERROR,
    SET_DONE
};

/* clang-format off */
static char const *const queries[] = {
    [INSERT] =\
"\
        INSERT INTO job\
            (\
                db_id, multi_hits, hmmer3_compat,      state,\
                error, submission, exec_started,  exec_ended\
            )\
        VALUES\
            (\
                ?, ?, ?, ?,\
                ?, ?, ?, ?\
            );\
",
    [GET_PEND] = \
"\
    SELECT id FROM job WHERE state = 'pend' ORDER BY id LIMIT 1;\
",
    [SET_RUN] = \
"\
        UPDATE job SET\
            state = 'run', exec_started = ?\
        WHERE \
            id = ?;\
",
    [GET_STATE] = "SELECT state FROM job WHERE id = ?;",
    [SELECT] = "SELECT * FROM job WHERE id = ?;\
",
    [SET_ERROR] = \
"\
    UPDATE job SET\
        state = 'fail', error = ?, exec_ended = ? WHERE id = ?;\
",
    [SET_DONE] = \
"\
    UPDATE job SET\
        state = 'done', exec_ended = ? WHERE id = ?;\
"};
/* clang-format on */

static struct sqlite3_stmt *stmts[ARRAY_SIZE(queries)] = {0};

enum rc job_module_init(void)
{
    for (unsigned i = 0; i < ARRAY_SIZE(queries); ++i)
    {
        if (xsql_prepare(sched, queries[i], stmts + i)) return RC_FAIL;
    }
    return RC_DONE;
}

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
    struct sqlite3_stmt *stmt = stmts[INSERT];
    if (xsql_reset(stmt)) return RC_FAIL;

    if (xsql_bind_i64(stmt, 0, job->db_id)) return RC_FAIL;
    if (xsql_bind_i64(stmt, 1, job->multi_hits)) return RC_FAIL;
    if (xsql_bind_i64(stmt, 2, job->hmmer3_compat)) return RC_FAIL;
    if (xsql_bind_str(stmt, 3, job->state)) return RC_FAIL;

    if (xsql_bind_str(stmt, 4, job->error)) return RC_FAIL;
    if (xsql_bind_i64(stmt, 5, job->submission)) return RC_FAIL;
    if (xsql_bind_i64(stmt, 6, job->exec_started)) return RC_FAIL;
    if (xsql_bind_i64(stmt, 7, job->exec_ended)) return RC_FAIL;

    if (xsql_step(stmt) != RC_DONE) return RC_FAIL;
    job->id = xsql_last_id(sched);
    return RC_DONE;
}

static int next_pending_job_id(int64_t *job_id)
{
    struct sqlite3_stmt *stmt = stmts[GET_PEND];
    if (xsql_reset(stmt)) return RC_FAIL;

    int rc = xsql_step(stmt);
    if (rc == RC_DONE) return RC_NOTFOUND;
    if (rc != RC_NEXT) return RC_FAIL;
    *job_id = sqlite3_column_int64(stmt, 0);
    if (xsql_step(stmt)) return RC_FAIL;

    stmt = stmts[SET_RUN];
    if (xsql_reset(stmt)) return RC_FAIL;

    if (xsql_bind_i64(stmt, 0, utc_now())) return RC_FAIL;
    if (xsql_bind_i64(stmt, 1, *job_id)) return RC_FAIL;
    return xsql_step(stmt) == RC_DONE ? RC_DONE : RC_FAIL;
}

enum rc job_next_pending(struct sched_job *job)
{
    int rc = next_pending_job_id(&job->id);
    if (rc == RC_NOTFOUND) return RC_NOTFOUND;
    if (rc != RC_DONE) return RC_FAIL;
    return job_get(job);
}

enum rc job_set_error(int64_t job_id, char const *error, int64_t exec_ended)
{
    struct sqlite3_stmt *stmt = stmts[SET_ERROR];
    if (xsql_reset(stmt)) return RC_FAIL;

    if (xsql_bind_str(stmt, 0, error)) return RC_FAIL;
    if (xsql_bind_i64(stmt, 1, exec_ended)) return RC_FAIL;
    if (xsql_bind_i64(stmt, 2, job_id)) return RC_FAIL;

    return xsql_end_step(stmt);
}

enum rc job_set_done(int64_t job_id, int64_t exec_ended)
{
    struct sqlite3_stmt *stmt = stmts[SET_DONE];
    if (xsql_reset(stmt)) return RC_FAIL;

    if (xsql_bind_i64(stmt, 0, exec_ended)) return RC_FAIL;
    if (xsql_bind_i64(stmt, 1, job_id)) return RC_FAIL;

    return xsql_end_step(stmt);
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

    exit(EXIT_FAILURE);
    return 0;
}

enum rc sched_job_state(int64_t job_id, enum sched_job_state *state)
{
    struct sqlite3_stmt *stmt = stmts[GET_STATE];
    if (xsql_reset(stmt)) return RC_FAIL;

    if (xsql_bind_i64(stmt, 0, job_id)) return RC_FAIL;

    int rc = xsql_step(stmt);
    if (rc == RC_DONE) return RC_NOTFOUND;
    if (rc != RC_NEXT) return RC_FAIL;

    char tmp[SCHED_JOB_STATE_SIZE] = {0};
    rc = xsql_cpy_txt(stmt, 0, (struct xsql_txt){SCHED_JOB_STATE_SIZE, tmp});
    if (rc) return RC_FAIL;
    *state = resolve_job_state(tmp);

    return xsql_end_step(stmt);
}

enum rc job_get(struct sched_job *job)
{
    struct sqlite3_stmt *stmt = stmts[SELECT];
    if (xsql_reset(stmt)) return RC_FAIL;

    if (xsql_bind_i64(stmt, 0, job->id)) return RC_FAIL;

    if (xsql_step(stmt) != RC_NEXT) return RC_FAIL;

    job->id = sqlite3_column_int64(stmt, 0);

    job->db_id = sqlite3_column_int64(stmt, 1);
    job->multi_hits = sqlite3_column_int(stmt, 2);
    job->hmmer3_compat = sqlite3_column_int(stmt, 3);
    if (xsql_cpy_txt(stmt, 4, XSQL_TXT_OF(*job, state))) return RC_FAIL;

    if (xsql_cpy_txt(stmt, 5, XSQL_TXT_OF(*job, error))) return RC_FAIL;
    job->submission = sqlite3_column_int64(stmt, 6);
    job->exec_started = sqlite3_column_int64(stmt, 7);
    job->exec_ended = sqlite3_column_int64(stmt, 8);

    return xsql_end_step(stmt);
}

void job_module_del(void)
{
    for (unsigned i = 0; i < ARRAY_SIZE(stmts); ++i)
        sqlite3_finalize(stmts[i]);
}
