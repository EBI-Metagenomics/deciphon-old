#include "sched_seq.h"
#include "compiler.h"
#include "logger.h"
#include "rc.h"
#include "safe.h"
#include "sched_macros.h"
#include "xsql.h"
#include <assert.h>
#include <limits.h>
#include <sqlite3.h>
#include <stdint.h>

enum
{
    INSERT,
    SELECT,
    SELECT_NEXT
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
    [SELECT] = "SELECT id, job_id, name, upper(data) FROM seq WHERE id = ?;\
",
    [SELECT_NEXT] = \
"\
        SELECT\
            id FROM seq\
        WHERE\
            id > ? AND job_id = ? ORDER BY id ASC LIMIT 1;\
"};
/* clang-format on */

static struct sqlite3_stmt *stmts[ARRAY_SIZE(queries)] = {0};

enum rc sched_seq_module_init(struct sqlite3 *db)
{
    enum rc rc = RC_DONE;
    for (unsigned i = 0; i < ARRAY_SIZE(queries); ++i)
        PREPARE_OR_CLEAN_UP(db, queries[i], stmts + i);

cleanup:
    return rc;
}

enum rc sched_seq_add(int64_t job_id, char const *name, unsigned len,
                      char const *data)
{
    struct sqlite3_stmt *stmt = stmts[INSERT];
    enum rc rc = RC_DONE;
    RESET_OR_CLEANUP(rc, stmt);

    if ((rc = xsql_bind_i64(stmt, 0, job_id))) goto cleanup;
    if ((rc = xsql_bind_str(stmt, 1, name))) goto cleanup;
    if ((rc = xsql_bind_txt(stmt, 2, (struct xsql_txt){len, data})))
        goto cleanup;

    STEP_OR_CLEANUP(stmt, SQLITE_ROW);
    if (sqlite3_step(stmt) != SQLITE_DONE) rc = STEP_ERROR();

cleanup:
    return rc;
}

enum rc sched_seq_next(int64_t job_id, int64_t *seq_id)
{
    struct sqlite3_stmt *stmt = stmts[SELECT_NEXT];
    enum rc rc = RC_DONE;
    RESET_OR_CLEANUP(rc, stmt);

    if ((rc = xsql_bind_i64(stmt, 0, *seq_id))) goto cleanup;
    if ((rc = xsql_bind_i64(stmt, 1, job_id))) goto cleanup;

    int code = sqlite3_step(stmt);
    if (code == SQLITE_DONE) return RC_DONE;
    if (code != SQLITE_ROW)
    {
        rc = STEP_ERROR();
        goto cleanup;
    }
    *seq_id = sqlite3_column_int64(stmt, 0);
    if (sqlite3_step(stmt) != SQLITE_DONE) rc = STEP_ERROR();

    return RC_NEXT;

cleanup:
    return rc;
}

enum rc sched_seq_get(struct sched_seq *seq, int64_t id)
{
    struct sqlite3_stmt *stmt = stmts[SELECT];
    enum rc rc = RC_DONE;
    RESET_OR_CLEANUP(rc, stmt);

    if ((rc = xsql_bind_i64(stmt, 0, id))) goto cleanup;

    STEP_OR_CLEANUP(stmt, SQLITE_ROW);

    seq->id = sqlite3_column_int64(stmt, 0);
    seq->job_id = sqlite3_column_int64(stmt, 1);

    if ((rc = xsql_cpy_txt(stmt, 2, XSQL_TXT_OF(*seq, name)))) goto cleanup;

    struct xsql_txt txt = {0};
    if ((rc = xsql_get_txt(stmt, 3, &txt))) goto cleanup;
    if ((rc = xsql_txt_as_array(&txt, &seq->data))) goto cleanup;

    STEP_OR_CLEANUP(stmt, SQLITE_DONE);

cleanup:
    return rc;
}

void sched_seq_module_del(void)
{
    for (unsigned i = 0; i < ARRAY_SIZE(stmts); ++i)
        sqlite3_finalize(stmts[i]);
}
