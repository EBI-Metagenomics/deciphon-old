#include "sched_db.h"
#include "logger.h"
#include "macros.h"
#include "rc.h"
#include "safe.h"
#include "sched_macros.h"
#include "xfile.h"
#include "xsql.h"
#include <sqlite3.h>
#include <stdlib.h>

enum stmt
{
    INSERT,
    SELECT_BY_ID,
    SELECT_BY_XXH64,
    EXIST
};

/* clang-format off */
static char const *const queries[] = {
    [INSERT] = \
"\
        INSERT INTO db\
            (\
                xxh64, name, filepath\
            )\
        VALUES\
            (\
                ?, ?, ?\
            )\
        RETURNING id;\
",
    [SELECT_BY_ID] = "SELECT * FROM db WHERE id = ?;\
",
    [SELECT_BY_XXH64] = "SELECT * FROM db WHERE xxh64 = ?;\
",
    [EXIST] = "SELECT COUNT(1) FROM db WHERE name = ?;\
"};
/* clang-format on */

static struct sqlite3_stmt *stmts[ARRAY_SIZE(queries)] = {0};

enum rc sched_db_setup(struct sched_db *db,
                           char const name[DB_NAME_SIZE],
                           char const filepath[DCP_PATH_SIZE])
{
    FILE *fd = fopen(filepath, "rb");
    if (!fd) return error(RC_IOERROR, "failed to open file");

    enum rc rc = xfile_hash(fd, (uint64_t *)&db->xxh64);
    if (rc) goto cleanup;

    safe_strcpy(db->name, name, DB_NAME_SIZE);
    safe_strcpy(db->filepath, filepath, DCP_PATH_SIZE);

cleanup:
    fclose(fd);
    return RC_DONE;
}

enum rc sched_db_module_init(struct sqlite3 *db)
{
    enum rc rc = RC_DONE;
    for (unsigned i = 0; i < ARRAY_SIZE(queries); ++i)
        PREPARE_OR_CLEAN_UP(db, queries[i], stmts + i);

cleanup:
    return rc;
}

enum rc sched_db_add(struct sched_db *db)
{
    struct sqlite3_stmt *stmt = stmts[INSERT];
    enum rc rc = RC_DONE;
    RESET_OR_CLEANUP(rc, stmt);

    BIND_INT64_OR_CLEANUP(rc, stmt, 1, db->xxh64);
    rc = xsql_get_text(stmt, 2, ARRAY_SIZE_OF(*db, name), db->name);
    if (rc) goto cleanup;
    rc = xsql_get_text(stmt, 3, ARRAY_SIZE_OF(*db, filepath), db->filepath);
    if (rc) goto cleanup;

    STEP_OR_CLEANUP(stmt, SQLITE_ROW);
    db->id = sqlite3_column_int64(stmt, 0);
    if (sqlite3_step(stmt) != SQLITE_DONE) rc = STEP_ERROR();

cleanup:
    return rc;
}

static enum rc select_db(struct sched_db *db, int64_t by_value,
                             enum stmt select_stmt)
{
    struct sqlite3_stmt *stmt = stmts[select_stmt];
    enum rc rc = RC_DONE;
    RESET_OR_CLEANUP(rc, stmt);

    BIND_INT64_OR_CLEANUP(rc, stmt, 1, by_value);
    int code = sqlite3_step(stmt);
    if (code == SQLITE_DONE) return RC_NOTFOUND;
    if (code != SQLITE_ROW)
    {
        rc = error(RC_FAIL, "failed to step");
        goto cleanup;
    }

    db->id = sqlite3_column_int64(stmt, 0);
    db->xxh64 = sqlite3_column_int64(stmt, 1);
    rc = xsql_get_text(stmt, 2, ARRAY_SIZE_OF(*db, name), db->name);
    if (rc) goto cleanup;
    rc = xsql_get_text(stmt, 3, ARRAY_SIZE_OF(*db, filepath), db->filepath);
    if (rc) goto cleanup;

    STEP_OR_CLEANUP(stmt, SQLITE_DONE);

cleanup:
    return rc;
}

enum rc sched_db_get_by_id(struct sched_db *db, int64_t id)
{
    return select_db(db, id, SELECT_BY_ID);
}

enum rc sched_db_get_by_xxh64(struct sched_db *db, int64_t xxh64)
{
    return select_db(db, xxh64, SELECT_BY_XXH64);
}

void sched_db_module_del(void)
{
    for (unsigned i = 0; i < ARRAY_SIZE(stmts); ++i)
        sqlite3_finalize(stmts[i]);
}
