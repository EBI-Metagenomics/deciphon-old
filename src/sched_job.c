#include "sched_job.h"
#include "compiler.h"
#include "job_state.h"
#include "logger.h"
#include "rc.h"
#include "safe.h"
#include "sched_limits.h"
#include "sched_macros.h"
#include "utc.h"
#include "xsql.h"
#include <sqlite3.h>
#include <stdlib.h>

enum
{
    INSERT,
    GET_PEND,
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
            )\
        RETURNING id;\
",
    [GET_PEND] = \
"\
        UPDATE job SET\
            state = 'run', exec_started = ?\
        WHERE \
            id = (SELECT MIN(id) FROM job WHERE state = 'pend' LIMIT 1)\
        RETURNING id;\
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

enum rc sched_job_module_init(struct sqlite3 *db)
{
    enum rc rc = RC_DONE;
    for (unsigned i = 0; i < ARRAY_SIZE(queries); ++i)
        PREPARE_OR_CLEAN_UP(db, queries[i], stmts + i);

cleanup:
    return rc;
}

enum rc sched_job_add(struct sched_job *job)
{
    struct sqlite3_stmt *stmt = stmts[INSERT];
    enum rc rc = RC_DONE;
    RESET_OR_CLEANUP(rc, stmt);

    if ((rc = xsql_bind_i64(stmt, 0, job->db_id))) goto cleanup;
    if ((rc = xsql_bind_i64(stmt, 1, job->multi_hits))) goto cleanup;
    if ((rc = xsql_bind_i64(stmt, 2, job->hmmer3_compat))) goto cleanup;
    if ((rc = xsql_bind_str(stmt, 3, job->state))) goto cleanup;

    if ((rc = xsql_bind_str(stmt, 4, job->error))) goto cleanup;
    if ((rc = xsql_bind_i64(stmt, 5, (int64_t)utc_now()))) goto cleanup;
    if ((rc = xsql_bind_i64(stmt, 6, 0))) goto cleanup;
    if ((rc = xsql_bind_i64(stmt, 7, 0))) goto cleanup;

    STEP_OR_CLEANUP(stmt, SQLITE_ROW);
    job->id = sqlite3_column_int64(stmt, 0);
    if (sqlite3_step(stmt) != SQLITE_DONE) rc = STEP_ERROR();

cleanup:
    return rc;
}

enum rc sched_job_next_pending(int64_t *job_id)
{
    struct sqlite3_stmt *stmt = stmts[GET_PEND];
    enum rc rc = RC_DONE;
    RESET_OR_CLEANUP(rc, stmt);

    if ((rc = xsql_bind_i64(stmt, 0, (int64_t)utc_now()))) goto cleanup;
    int code = sqlite3_step(stmt);
    if (code == SQLITE_DONE) return RC_NOTFOUND;
    if (code != SQLITE_ROW)
    {
        rc = STEP_ERROR();
        goto cleanup;
    }
    *job_id = sqlite3_column_int64(stmt, 0);
    STEP_OR_CLEANUP(stmt, SQLITE_DONE);

cleanup:
    return rc;
}

enum rc sched_job_set_error(int64_t job_id, char const *error,
                            int64_t exec_ended)
{
    struct sqlite3_stmt *stmt = stmts[SET_ERROR];
    enum rc rc = RC_DONE;
    RESET_OR_CLEANUP(rc, stmt);

    if ((rc = xsql_bind_str(stmt, 0, error))) goto cleanup;
    if ((rc = xsql_bind_i64(stmt, 1, exec_ended))) goto cleanup;
    if ((rc = xsql_bind_i64(stmt, 2, job_id))) goto cleanup;

    STEP_OR_CLEANUP(stmt, SQLITE_DONE);

cleanup:
    return rc;
}

enum rc sched_job_set_done(int64_t job_id, int64_t exec_ended)
{
    struct sqlite3_stmt *stmt = stmts[SET_DONE];
    enum rc rc = RC_DONE;
    RESET_OR_CLEANUP(rc, stmt);

    if ((rc = xsql_bind_i64(stmt, 0, exec_ended))) goto cleanup;
    if ((rc = xsql_bind_i64(stmt, 1, job_id))) goto cleanup;

    STEP_OR_CLEANUP(stmt, SQLITE_DONE);

cleanup:
    return rc;
}

enum rc sched_job_state(int64_t job_id, enum job_state *state)
{
    struct sqlite3_stmt *stmt = stmts[GET_STATE];
    enum rc rc = RC_DONE;
    RESET_OR_CLEANUP(rc, stmt);

    if ((rc = xsql_bind_i64(stmt, 0, job_id))) goto cleanup;
    int code = sqlite3_step(stmt);
    if (code == SQLITE_DONE) return RC_NOTFOUND;
    if (code != SQLITE_ROW)
    {
        rc = STEP_ERROR();
        goto cleanup;
    }

    char tmp[SCHED_JOB_STATE_SIZE] = {0};
    rc = xsql_cpy_txt(stmt, 0, (struct xsql_txt){SCHED_JOB_STATE_SIZE, tmp});
    if (rc) goto cleanup;
    *state = job_state_resolve(tmp);
    STEP_OR_CLEANUP(stmt, SQLITE_DONE);

cleanup:
    return rc;
}

enum rc sched_job_get(struct sched_job *job, int64_t job_id)
{
    struct sqlite3_stmt *stmt = stmts[SELECT];
    enum rc rc = RC_DONE;
    RESET_OR_CLEANUP(rc, stmt);

    if ((rc = xsql_bind_i64(stmt, 0, job_id))) goto cleanup;
    STEP_OR_CLEANUP(stmt, SQLITE_ROW);

    job->id = sqlite3_column_int64(stmt, 0);

    job->db_id = sqlite3_column_int64(stmt, 1);
    job->multi_hits = sqlite3_column_int(stmt, 2);
    job->hmmer3_compat = sqlite3_column_int(stmt, 3);
    rc = xsql_cpy_txt(stmt, 4, XSQL_TXT_OF(*job, state));
    if (rc) goto cleanup;

    rc = xsql_cpy_txt(stmt, 5, XSQL_TXT_OF(*job, error));
    if (rc) goto cleanup;
    job->submission = sqlite3_column_int64(stmt, 6);
    job->exec_started = sqlite3_column_int64(stmt, 7);
    job->exec_ended = sqlite3_column_int64(stmt, 8);

    STEP_OR_CLEANUP(stmt, SQLITE_DONE);

cleanup:
    return rc;
}

void sched_job_module_del(void)
{
    for (unsigned i = 0; i < ARRAY_SIZE(stmts); ++i)
        sqlite3_finalize(stmts[i]);
}
