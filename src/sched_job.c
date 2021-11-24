#include "sched_job.h"
#include "dcp/rc.h"
#include "error.h"
#include "job_state.h"
#include "macros.h"
#include "sched_macros.h"
#include "utc.h"
#include "xstrlcpy.h"
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
            id = (SELECT MIN(id) FROM job WHERE state = 'pend')\
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

enum dcp_rc sched_job_module_init(struct sqlite3 *db)
{
    enum dcp_rc rc = DCP_DONE;
    for (unsigned i = 0; i < ARRAY_SIZE(queries); ++i)
        PREPARE_OR_CLEAN_UP(db, queries[i], stmts + i);

cleanup:
    return rc;
}

enum dcp_rc sched_job_add(struct sched_job *job)
{
    struct sqlite3_stmt *stmt = stmts[INSERT];
    enum dcp_rc rc = DCP_DONE;
    RESET_OR_CLEANUP(rc, stmt);

    BIND_INT64_OR_CLEANUP(rc, stmt, 1, job->db_id);
    BIND_INT_OR_CLEANUP(rc, stmt, 2, job->multi_hits);
    BIND_INT_OR_CLEANUP(rc, stmt, 3, job->hmmer3_compat);
    BIND_TEXT_OR_CLEANUP(rc, stmt, 4, job->state);

    BIND_TEXT_OR_CLEANUP(rc, stmt, 5, job->error);
    BIND_INT64_OR_CLEANUP(rc, stmt, 6, (int64_t)utc_now());
    BIND_INT64_OR_CLEANUP(rc, stmt, 7, 0);
    BIND_INT64_OR_CLEANUP(rc, stmt, 8, 0);

    STEP_OR_CLEANUP(stmt, SQLITE_ROW);
    job->id = sqlite3_column_int64(stmt, 0);
    if (sqlite3_step(stmt) != SQLITE_DONE) rc = STEP_ERROR();

cleanup:
    return rc;
}

enum dcp_rc sched_job_next_pending(int64_t *job_id)
{
    struct sqlite3_stmt *stmt = stmts[GET_PEND];
    enum dcp_rc rc = DCP_DONE;
    RESET_OR_CLEANUP(rc, stmt);

    BIND_INT64_OR_CLEANUP(rc, stmt, 1, (int64_t)utc_now());
    int code = sqlite3_step(stmt);
    if (code == SQLITE_DONE) return DCP_NOTFOUND;
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

enum dcp_rc sched_job_set_error(int64_t job_id,
                                char const error[DCP_ERROR_SIZE],
                                int64_t exec_ended)
{
    struct sqlite3_stmt *stmt = stmts[SET_ERROR];
    enum dcp_rc rc = DCP_DONE;
    RESET_OR_CLEANUP(rc, stmt);

    BIND_TEXT_OR_CLEANUP(rc, stmt, 1, error);
    BIND_INT64_OR_CLEANUP(rc, stmt, 2, exec_ended);
    BIND_INT64_OR_CLEANUP(rc, stmt, 3, job_id);

    STEP_OR_CLEANUP(stmt, SQLITE_DONE);

cleanup:
    return rc;
}

enum dcp_rc sched_job_set_done(int64_t job_id, int64_t exec_ended)
{
    struct sqlite3_stmt *stmt = stmts[SET_DONE];
    enum dcp_rc rc = DCP_DONE;
    RESET_OR_CLEANUP(rc, stmt);

    BIND_INT64_OR_CLEANUP(rc, stmt, 1, exec_ended);
    BIND_INT64_OR_CLEANUP(rc, stmt, 2, job_id);

    STEP_OR_CLEANUP(stmt, SQLITE_DONE);

cleanup:
    return rc;
}

enum dcp_rc sched_job_state(int64_t job_id, enum dcp_job_state *state)
{
    struct sqlite3_stmt *stmt = stmts[GET_STATE];
    enum dcp_rc rc = DCP_DONE;
    RESET_OR_CLEANUP(rc, stmt);

    BIND_INT64_OR_CLEANUP(rc, stmt, 1, job_id);
    int code = sqlite3_step(stmt);
    if (code == SQLITE_DONE) return DCP_NOTFOUND;
    if (code != SQLITE_ROW)
    {
        rc = STEP_ERROR();
        goto cleanup;
    }

    char tmp[DCP_STATE_SIZE] = {0};
    COLUMN_TEXT(stmt, 0, tmp, DCP_STATE_SIZE);
    *state = job_state_resolve(tmp);
    STEP_OR_CLEANUP(stmt, SQLITE_DONE);

cleanup:
    return rc;
}

enum dcp_rc sched_job_get(struct sched_job *job, int64_t job_id)
{
    struct sqlite3_stmt *stmt = stmts[SELECT];
    enum dcp_rc rc = DCP_DONE;
    RESET_OR_CLEANUP(rc, stmt);

    BIND_INT64_OR_CLEANUP(rc, stmt, 1, job_id);
    STEP_OR_CLEANUP(stmt, SQLITE_ROW);

    job->id = sqlite3_column_int64(stmt, 0);

    job->db_id = sqlite3_column_int64(stmt, 1);
    job->multi_hits = sqlite3_column_int(stmt, 2);
    job->hmmer3_compat = sqlite3_column_int(stmt, 3);
    COLUMN_TEXT(stmt, 4, job->state, ARRAY_SIZE(MEMBER_REF(*job, state)));

    COLUMN_TEXT(stmt, 5, job->error, ARRAY_SIZE(MEMBER_REF(*job, error)));
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
