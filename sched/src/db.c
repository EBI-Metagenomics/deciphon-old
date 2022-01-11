#include "db.h"
#include "common/logger.h"
#include "common/rc.h"
#include "common/safe.h"
#include "common/xfile.h"
#include "sched/sched.h"
#include "stmt.h"
#include "xsql.h"
#include <sqlite3.h>
#include <stdlib.h>

extern struct sqlite3 *sched;

static enum rc init_db(struct db *db, char const *filepath)
{
    FILE *fp = fopen(filepath, "rb");
    if (!fp) return failed_to(RC_IOERROR, "fopen");

    enum rc rc = xfile_hash(fp, (uint64_t *)&db->xxh64);
    if (rc) goto cleanup;

    safe_strcpy(db->filepath, filepath, SCHED_PATH_SIZE);

cleanup:
    fclose(fp);
    return rc;
}

enum rc db_add(char const *filepath, int64_t *id)
{
    struct sqlite3_stmt *st = stmt[DB_INSERT];
    struct db db = {0};

    enum rc rc = init_db(&db, filepath);
    if (rc) return rc;

    if (xsql_reset(st)) return failed_to(RC_FAIL, "reset");

    if (xsql_bind_i64(st, 0, db.xxh64)) return failed_to(RC_FAIL, "bind");
    if (xsql_bind_txt(st, 1, XSQL_TXT_OF(db, filepath)))
        return failed_to(RC_FAIL, "bind");

    if (xsql_step(st) != RC_DONE) return failed_to(RC_FAIL, "add db");
    *id = xsql_last_id(sched);
    return RC_DONE;
}

enum rc db_has(char const *filepath, struct db *db)
{
    enum rc rc = init_db(db, filepath);
    if (rc) return rc;
    return db_get_by_xxh64(db, db->xxh64);
}

enum rc db_hash(char const *filepath, int64_t *xxh64)
{
    struct db db = {0};
    enum rc rc = init_db(&db, filepath);
    *xxh64 = db.xxh64;
    return rc;
}

static enum rc select_db(struct db *db, int64_t by_value, enum stmt select_stmt)
{
    struct sqlite3_stmt *st = stmt[select_stmt];
    if (xsql_reset(st)) return failed_to(RC_FAIL, "reset");

    if (xsql_bind_i64(st, 0, by_value)) return failed_to(RC_FAIL, "bind");

    int rc = xsql_step(st);
    if (rc == RC_DONE) return RC_NOTFOUND;
    if (rc != RC_NEXT) return failed_to(RC_FAIL, "get db");

    db->id = sqlite3_column_int64(st, 0);
    db->xxh64 = sqlite3_column_int64(st, 1);
    if (xsql_cpy_txt(st, 2, XSQL_TXT_OF(*db, filepath))) return RC_DONE;

    if (xsql_step(st)) return failed_to(RC_FAIL, "step");
    return RC_DONE;
}

enum rc db_get_by_id(struct db *db, int64_t id)
{
    return select_db(db, id, DB_SELECT_BY_ID);
}

enum rc db_get_by_xxh64(struct db *db, int64_t xxh64)
{
    return select_db(db, xxh64, DB_SELECT_BY_XXH64);
}
