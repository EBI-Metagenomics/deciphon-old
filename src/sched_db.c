#include "sched_db.h"
#include "dcp/rc.h"
#include "error.h"
#include "macros.h"
#include "sched_macros.h"
#include "xfile.h"
#include "xstrlcpy.h"
#include <sqlite3.h>
#include <stdlib.h>

enum
{
    INSERT,
    SELECT,
    EXIST
};

/* clang-format off */
static char const *const queries[] = {
    [INSERT] = \
"\
        INSERT INTO db\
            (\
                id, name, filepath\
            )\
        VALUES\
            (\
                ?, ?, ?\
            );\
",
    [SELECT] = "SELECT * FROM db WHERE id = ?;\
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

    enum dcp_rc rc = xfile_hash(fd, (uint64_t *)&db->id);
    if (rc) goto cleanup;

    xstrlcpy(db->name, name, DCP_DB_NAME_SIZE);
    xstrlcpy(db->filepath, filepath, DCP_PATH_SIZE);

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

    BIND_INT64_OR_CLEANUP(rc, stmt, 1, db->id);
    BIND_TEXT_OR_CLEANUP(rc, stmt, 2, db->name);
    BIND_TEXT_OR_CLEANUP(rc, stmt, 3, db->filepath);

    STEP_OR_CLEANUP(stmt, SQLITE_DONE);

cleanup:
    return rc;
}

enum dcp_rc sched_db_get(struct sched_db *db, int64_t db_id)
{
    struct sqlite3_stmt *stmt = stmts[SELECT];
    enum dcp_rc rc = DCP_DONE;
    RESET_OR_CLEANUP(rc, stmt);

    BIND_INT64_OR_CLEANUP(rc, stmt, 1, db_id);
    STEP_OR_CLEANUP(stmt, SQLITE_ROW);

    db->id = sqlite3_column_int64(stmt, 0);
    COLUMN_TEXT(stmt, 1, db->name, ARRAY_SIZE(MEMBER_REF(*db, name)));
    COLUMN_TEXT(stmt, 2, db->filepath, ARRAY_SIZE(MEMBER_REF(*db, filepath)));

    STEP_OR_CLEANUP(stmt, SQLITE_DONE);

cleanup:
    return rc;
}

void sched_db_module_del(void)
{
    for (unsigned i = 0; i < ARRAY_SIZE(stmts); ++i)
        sqlite3_finalize(stmts[i]);
}
