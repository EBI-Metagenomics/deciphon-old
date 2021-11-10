#include "sched_prod.h"
#include "error.h"
#include "macros.h"
#include "sched_limits.h"
#include "sched_macros.h"
#include "xstrlcpy.h"
#include <sqlite3.h>
#include <stdlib.h>

enum
{
    INSERT,
    SELECT
};

/* clang-format off */
static char const *const queries[] = {
    [INSERT] = \
"\
        INSERT INTO prod\
            (\
                job_id,       seq_id, match_id,  prof_name,\
                start_pos,   end_pos, abc_id,       loglik,\
                null_loglik,   model, version,  match_data\
            )\
        VALUES\
            (\
                ?, ?, ?, ?,\
                ?, ?, ?, ?,\
                ?, ?, ?, ?\
            ) RETURNING id;\
",
    [SELECT] = "SELECT * FROM seq WHERE id = ?;\
"};
/* clang-format on */

static struct sqlite3_stmt *stmts[ARRAY_SIZE(queries)] = {0};

enum dcp_rc sched_prod_module_init(struct sqlite3 *db)
{
    enum dcp_rc rc = DCP_DONE;
    for (unsigned i = 0; i < ARRAY_SIZE(queries); ++i)
        PREPARE_OR_CLEAN_UP(db, queries[i], stmts + i);

cleanup:
    return rc;
}

enum dcp_rc sched_prod_add(struct sched_prod *prod)
{
    struct sqlite3_stmt *stmt = stmts[INSERT];
    enum dcp_rc rc = DCP_DONE;
    RESET_OR_CLEANUP(rc, stmt);

    BIND_INT64_OR_CLEANUP(rc, stmt, 1, prod->job_id);
    BIND_INT64_OR_CLEANUP(rc, stmt, 2, prod->seq_id);
    BIND_INT64_OR_CLEANUP(rc, stmt, 3, prod->match_id);
    BIND_TEXT_OR_CLEANUP(rc, stmt, 4, prod->prof_name);

    BIND_INT64_OR_CLEANUP(rc, stmt, 5, prod->start_pos);
    BIND_INT64_OR_CLEANUP(rc, stmt, 6, prod->end_pos);
    BIND_TEXT_OR_CLEANUP(rc, stmt, 7, prod->abc_id);
    BIND_DOUBLE_OR_CLEANUP(rc, stmt, 8, prod->loglik);

    BIND_DOUBLE_OR_CLEANUP(rc, stmt, 9, prod->null_loglik);
    BIND_TEXT_OR_CLEANUP(rc, stmt, 10, prod->model);
    BIND_TEXT_OR_CLEANUP(rc, stmt, 11, prod->version);
    BIND_TEXT_OR_CLEANUP(rc, stmt, 12, prod->match_data);

    STEP_OR_CLEANUP(stmt, SQLITE_ROW);
    prod->id = sqlite3_column_int64(stmt, 0);
    if (sqlite3_step(stmt) != SQLITE_DONE) rc = STEP_ERROR();

cleanup:
    return rc;
}

enum dcp_rc sched_prod_get(struct sched_prod *prod, int64_t prod_id)
{
    struct sqlite3_stmt *stmt = stmts[SELECT];
    enum dcp_rc rc = DCP_DONE;
    RESET_OR_CLEANUP(rc, stmt);

    BIND_INT64_OR_CLEANUP(rc, stmt, 1, prod_id);
    STEP_OR_CLEANUP(stmt, SQLITE_ROW);

    prod->id = sqlite3_column_int64(stmt, 0);

    prod->job_id = sqlite3_column_int64(stmt, 1);
    prod->seq_id = sqlite3_column_int64(stmt, 2);
    prod->match_id = sqlite3_column_int64(stmt, 3);
    COLUMN_TEXT(stmt, 4, prod->prof_name,
                ARRAY_SIZE(MEMBER_REF(*prod, prof_name)));

    prod->start_pos = sqlite3_column_int64(stmt, 5);
    prod->end_pos = sqlite3_column_int64(stmt, 6);
    COLUMN_TEXT(stmt, 7, prod->abc_id, ARRAY_SIZE(MEMBER_REF(*prod, abc_id)));
    prod->loglik = sqlite3_column_double(stmt, 8);

    prod->null_loglik = sqlite3_column_double(stmt, 9);
    COLUMN_TEXT(stmt, 10, prod->model, ARRAY_SIZE(MEMBER_REF(*prod, model)));
    COLUMN_TEXT(stmt, 11, prod->version,
                ARRAY_SIZE(MEMBER_REF(*prod, version)));
    COLUMN_TEXT(stmt, 12, prod->match_data,
                ARRAY_SIZE(MEMBER_REF(*prod, match_data)));

    STEP_OR_CLEANUP(stmt, SQLITE_DONE);

cleanup:
    return rc;
}

void sched_prod_module_del(void)
{
    for (unsigned i = 0; i < ARRAY_SIZE(stmts); ++i)
        sqlite3_finalize(stmts[i]);
}
