#include "sched_seq.h"
#include "dcp/rc.h"
#include "error.h"
#include "macros.h"
#include "sched_limits.h"
#include "sched_macros.h"
#include "xstrlcpy.h"
#include <sqlite3.h>
#include <stdint.h>

enum
{
    INSERT,
    SELECT,
    NEXT
};

/* clang-format off */
static char const *const queries[] = {
    [INSERT] = \
"\
    INSERT INTO seq\
        (\
            job_id, name, data\
        )\
    VALUES\
        (\
            ?, ?, ?\
        ) RETURNING id;\
",
    [SELECT] = "SELECT * FROM seq WHERE id = ?;\
",
    [NEXT] = "SELECT id FROM seq WHERE id > ? AND job_id = ?;"};
/* clang-format on */

static struct sqlite3_stmt *stmts[ARRAY_SIZE(queries)] = {0};

enum dcp_rc sched_seq_module_init(struct sqlite3 *db)
{
    enum dcp_rc rc = DCP_DONE;
    for (unsigned i = 0; i < ARRAY_SIZE(queries); ++i)
        PREPARE_OR_CLEAN_UP(db, queries[i], stmts + i);

cleanup:
    return rc;
}

void sched_seq_setup(struct sched_seq *seq, char const name[SCHED_NAME_SIZE],
                     char const data[SCHED_DATA_SIZE])
{
    xstrlcpy(seq->name, name, SCHED_NAME_SIZE);
    xstrlcpy(seq->data, data, SCHED_DATA_SIZE);
}

enum dcp_rc sched_seq_add(struct sched_seq *seq)
{
    struct sqlite3_stmt *stmt = stmts[INSERT];
    enum dcp_rc rc = DCP_DONE;
    RESET_OR_CLEANUP(rc, stmt);

    BIND_INT64_OR_CLEANUP(rc, stmt, 1, seq->job_id);
    BIND_TEXT_OR_CLEANUP(rc, stmt, 2, seq->name);
    BIND_TEXT_OR_CLEANUP(rc, stmt, 3, seq->data);

    STEP_OR_CLEANUP(stmt, SQLITE_ROW);
    seq->id = sqlite3_column_int64(stmt, 0);
    if (sqlite3_step(stmt) != SQLITE_DONE) rc = STEP_ERROR();

cleanup:
    return rc;
}

enum dcp_rc sched_seq_next(int64_t job_id, int64_t *seq_id)
{
    struct sqlite3_stmt *stmt = stmts[NEXT];
    enum dcp_rc rc = DCP_DONE;
    RESET_OR_CLEANUP(rc, stmt);

    BIND_INT64_OR_CLEANUP(rc, stmt, 1, *seq_id);
    BIND_INT64_OR_CLEANUP(rc, stmt, 2, job_id);

    int code = sqlite3_step(stmt);
    if (code == SQLITE_DONE) return DCP_DONE;
    if (code != SQLITE_ROW)
    {
        rc = STEP_ERROR();
        goto cleanup;
    }
    *seq_id = sqlite3_column_int64(stmt, 0);
    if (sqlite3_step(stmt) != SQLITE_DONE) rc = STEP_ERROR();

    return DCP_NEXT;

cleanup:
    return rc;
}

enum dcp_rc sched_seq_get(struct sched_seq *seq, int64_t id)
{
    struct sqlite3_stmt *stmt = stmts[SELECT];
    enum dcp_rc rc = DCP_DONE;
    RESET_OR_CLEANUP(rc, stmt);

    BIND_INT64_OR_CLEANUP(rc, stmt, 1, id);

    STEP_OR_CLEANUP(stmt, SQLITE_ROW);

    seq->id = sqlite3_column_int64(stmt, 0);
    seq->job_id = sqlite3_column_int64(stmt, 1);
    BIND_TEXT_OR_CLEANUP(rc, stmt, 2, seq->name);
    BIND_TEXT_OR_CLEANUP(rc, stmt, 3, seq->data);

    STEP_OR_CLEANUP(stmt, SQLITE_DONE);

cleanup:
    return rc;
}

void sched_seq_module_del(void)
{
    for (unsigned i = 0; i < ARRAY_SIZE(stmts); ++i)
        sqlite3_finalize(stmts[i]);
}
