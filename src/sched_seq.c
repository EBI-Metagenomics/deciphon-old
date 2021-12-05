#include "sched_seq.h"
#include "logger.h"
#include "macros.h"
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

    BIND_INT64_OR_CLEANUP(rc, stmt, 1, job_id);
    BIND_STRING_OR_CLEANUP(rc, stmt, 2, name);
    BIND_TEXT_OR_CLEANUP(rc, stmt, 3, (int)len, data);

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

    BIND_INT64_OR_CLEANUP(rc, stmt, 1, *seq_id);
    BIND_INT64_OR_CLEANUP(rc, stmt, 2, job_id);

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

    BIND_INT64_OR_CLEANUP(rc, stmt, 1, id);

    STEP_OR_CLEANUP(stmt, SQLITE_ROW);

    seq->id = sqlite3_column_int64(stmt, 0);
    seq->job_id = sqlite3_column_int64(stmt, 1);

    rc = xsql_get_text(stmt, 2, ARRAY_SIZE_OF(*seq, name), seq->name);
    if (rc) goto cleanup;

    rc = xsql_get_text_as_array(stmt, 3, &seq->data);
    if (rc) goto cleanup;

    STEP_OR_CLEANUP(stmt, SQLITE_DONE);

cleanup:
    return rc;
}

void sched_seq_module_del(void)
{
    for (unsigned i = 0; i < ARRAY_SIZE(stmts); ++i)
        sqlite3_finalize(stmts[i]);
}
