#include "sched_seq.h"
#include "compiler.h"
#include "logger.h"
#include "rc.h"
#include "safe.h"
#include "sched.h"
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

enum rc sched_seq_module_init(void)
{
    enum rc rc = RC_DONE;
    for (unsigned i = 0; i < ARRAY_SIZE(queries); ++i)
    {
        if ((rc = xsql_prepare(sched, queries[i], stmts + i))) return rc;
    }
    return RC_DONE;
}

enum rc sched_seq_add(int64_t job_id, char const *name, unsigned len,
                      char const *data)
{
    struct sqlite3_stmt *stmt = stmts[INSERT];
    enum rc rc = RC_DONE;
    if ((rc = xsql_reset(stmt))) return rc;

    if ((rc = xsql_bind_i64(stmt, 0, job_id))) return rc;
    if ((rc = xsql_bind_str(stmt, 1, name))) return rc;
    if ((rc = xsql_bind_txt(stmt, 2, (struct xsql_txt){len, data}))) return rc;

    return xsql_insert_step(stmt);
}

enum rc sched_seq_next(int64_t job_id, int64_t *seq_id)
{
    struct sqlite3_stmt *stmt = stmts[SELECT_NEXT];
    enum rc rc = RC_DONE;
    if ((rc = xsql_reset(stmt))) return rc;

    if ((rc = xsql_bind_i64(stmt, 0, *seq_id))) return rc;
    if ((rc = xsql_bind_i64(stmt, 1, job_id))) return rc;

    rc = xsql_step(stmt);
    if (rc == RC_DONE) return rc;
    if (rc != RC_NEXT) return rc;
    *seq_id = sqlite3_column_int64(stmt, 0);

    return xsql_end_step(stmt);
}

enum rc sched_seq_get(struct sched_seq *seq, int64_t id)
{
    struct sqlite3_stmt *stmt = stmts[SELECT];
    enum rc rc = RC_DONE;
    if ((rc = xsql_reset(stmt))) return rc;

    if ((rc = xsql_bind_i64(stmt, 0, id))) return rc;

    rc = xsql_step(stmt);
    if (rc != RC_NEXT) return error(RC_FAIL, "failed to get seq");

    seq->id = sqlite3_column_int64(stmt, 0);
    seq->job_id = sqlite3_column_int64(stmt, 1);

    if ((rc = xsql_cpy_txt(stmt, 2, XSQL_TXT_OF(*seq, name)))) return rc;

    struct xsql_txt txt = {0};
    if ((rc = xsql_get_txt(stmt, 3, &txt))) return rc;
    if ((rc = xsql_txt_as_array(&txt, &seq->data))) return rc;

    return xsql_end_step(stmt);
}

void sched_seq_module_del(void)
{
    for (unsigned i = 0; i < ARRAY_SIZE(stmts); ++i)
        sqlite3_finalize(stmts[i]);
}
