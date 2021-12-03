#include "sched_db.h"
#include "dcp/rc.h"
#include "error.h"
#include "macros.h"
#include "sched_macros.h"
#include "xfile.h"
#include "xstrlcpy.h"
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

enum dcp_rc sched_db_setup(struct sched_db *db,
                           char const name[DCP_DB_NAME_SIZE],
                           char const filepath[DCP_PATH_SIZE])
{
    FILE *fd = fopen(filepath, "rb");
    if (!fd) return error(DCP_IOERROR, "failed to open file");

    enum dcp_rc rc = xfile_hash(fd, (uint64_t *)&db->xxh64);
    if (rc) goto cleanup;

    safe_strcpy(db->name, name, DCP_DB_NAME_SIZE);
    safe_strcpy(db->filepath, filepath, DCP_PATH_SIZE);

cleanup:
    fclose(fd);
    return DCP_DONE;
}

enum dcp_rc sched_db_module_init(struct sqlite3 *db)
{
    enum dcp_rc rc = DCP_DONE;
    for (unsigned i = 0; i < ARRAY_SIZE(queries); ++i)
        PREPARE_OR_CLEAN_UP(db, queries[i], stmts + i);

cleanup:
    return rc;
}

enum dcp_rc sched_db_add(struct sched_db *db)
{
    struct sqlite3_stmt *stmt = stmts[INSERT];
    enum dcp_rc rc = DCP_DONE;
    RESET_OR_CLEANUP(rc, stmt);

    BIND_INT64_OR_CLEANUP(rc, stmt, 1, db->xxh64);
    BIND_TEXT_OR_CLEANUP(rc, stmt, 2, db->name);
    BIND_TEXT_OR_CLEANUP(rc, stmt, 3, db->filepath);

    STEP_OR_CLEANUP(stmt, SQLITE_ROW);
    db->id = sqlite3_column_int64(stmt, 0);
    if (sqlite3_step(stmt) != SQLITE_DONE) rc = STEP_ERROR();

cleanup:
    return rc;
}

static enum dcp_rc select_db(struct sched_db *db, int64_t by_value,
                             enum stmt select_stmt)
{
    struct sqlite3_stmt *stmt = stmts[select_stmt];
    enum dcp_rc rc = DCP_DONE;
    RESET_OR_CLEANUP(rc, stmt);

    BIND_INT64_OR_CLEANUP(rc, stmt, 1, by_value);
    int code = sqlite3_step(stmt);
    if (code == SQLITE_DONE) return DCP_NOTFOUND;
    if (code != SQLITE_ROW)
    {
        rc = error(DCP_FAIL, "failed to step");
        goto cleanup;
    }

    db->id = sqlite3_column_int64(stmt, 0);
    db->xxh64 = sqlite3_column_int64(stmt, 1);
    COLUMN_TEXT(stmt, 2, db->name, ARRAY_SIZE(MEMBER_REF(*db, name)));
    COLUMN_TEXT(stmt, 3, db->filepath, ARRAY_SIZE(MEMBER_REF(*db, filepath)));

    STEP_OR_CLEANUP(stmt, SQLITE_DONE);

cleanup:
    return rc;
}

enum dcp_rc sched_db_get_by_id(struct sched_db *db, int64_t id)
{
    return select_db(db, id, SELECT_BY_ID);
}

enum dcp_rc sched_db_get_by_xxh64(struct sched_db *db, int64_t xxh64)
{
    return select_db(db, xxh64, SELECT_BY_XXH64);
}

void sched_db_module_del(void)
{
    for (unsigned i = 0; i < ARRAY_SIZE(stmts); ++i)
        sqlite3_finalize(stmts[i]);
}
