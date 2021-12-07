#include "sched_db.h"
#include "compiler.h"
#include "logger.h"
#include "rc.h"
#include "safe.h"
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

enum rc sched_db_setup(struct sched_db *db, char const name[DCP_DB_NAME_SIZE],
                       char const filepath[DCP_PATH_SIZE])
{
    FILE *fp = fopen(filepath, "rb");
    if (!fp) return error(RC_IOERROR, "failed to open file");

    enum rc rc = xfile_hash(fp, (uint64_t *)&db->xxh64);
    if (rc) goto cleanup;

    safe_strcpy(db->name, name, DCP_DB_NAME_SIZE);
    safe_strcpy(db->filepath, filepath, DCP_PATH_SIZE);

cleanup:
    fclose(fp);
    return RC_DONE;
}

enum rc sched_db_module_init(struct sqlite3 *db)
{
    enum rc rc = RC_DONE;
    for (unsigned i = 0; i < ARRAY_SIZE(queries); ++i)
    {
        if ((rc = xsql_prepare(db, queries[i], stmts + i))) return rc;
    }
    return RC_DONE;
}

enum rc sched_db_add(struct sched_db *db)
{
    struct sqlite3_stmt *stmt = stmts[INSERT];
    enum rc rc = RC_DONE;
    if ((rc = xsql_reset(stmt))) return rc;

    if ((rc = xsql_bind_i64(stmt, 0, db->xxh64))) return rc;
    if ((rc = xsql_bind_txt(stmt, 1, XSQL_TXT_OF(*db, name)))) return rc;
    if ((rc = xsql_bind_txt(stmt, 2, XSQL_TXT_OF(*db, filepath)))) return rc;

    rc = xsql_step(stmt);
    if (rc != RC_NEXT) return rc;
    db->id = sqlite3_column_int64(stmt, 0);
    return xsql_end_step(stmt);
}

static enum rc select_db(struct sched_db *db, int64_t by_value,
                         enum stmt select_stmt)
{
    struct sqlite3_stmt *stmt = stmts[select_stmt];
    enum rc rc = RC_DONE;
    if ((rc = xsql_reset(stmt))) return rc;

    if ((rc = xsql_bind_i64(stmt, 0, by_value))) return rc;

    rc = xsql_step(stmt);
    if (rc == RC_DONE) return RC_NOTFOUND;
    if (rc != RC_NEXT) return rc;

    db->id = sqlite3_column_int64(stmt, 0);
    db->xxh64 = sqlite3_column_int64(stmt, 1);
    if ((rc = xsql_cpy_txt(stmt, 2, XSQL_TXT_OF(*db, name)))) return rc;
    if ((rc = xsql_cpy_txt(stmt, 3, XSQL_TXT_OF(*db, filepath)))) return rc;

    return xsql_end_step(stmt);
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
