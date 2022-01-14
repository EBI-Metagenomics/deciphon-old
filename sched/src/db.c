#include "db.h"
#include "common/logger.h"
#include "common/rc.h"
#include "common/safe.h"
#include "common/xfile.h"
#include "sched/db.h"
#include "sched/sched.h"
#include "stmt.h"
#include "xsql.h"
#include <sqlite3.h>
#include <stdlib.h>

extern struct sqlite3 *sched;

enum rc sched_db_list(sched_db_peek_t *peek, void *arg)
{
    struct db db = {0};
    struct sched_db sched_db = {0};

    struct sqlite3_stmt *st = stmt[DB_SELECT_ALL];
    if (xsql_reset(st)) return efail("reset");

    enum rc rc = RC_EFAIL;
    while ((rc = xsql_step(st)) == RC_NEXT)
    {
        db.id = sqlite3_column_int64(st, 0);
        if (xsql_cpy_txt(st, 2, XSQL_TXT_OF(db, filepath)))
            return efail("copy txt");

        sched_db.id = db.id;
        db_name(sched_db.name, db.filepath);
        peek(&sched_db, arg);
    }
    if (rc != RC_DONE) return efail("list databases");

    return rc;
}

static enum rc init_db(struct db *db, char const *filepath)
{
    FILE *fp = fopen(filepath, "rb");
    if (!fp) return eio("fopen");

    enum rc rc = xfile_hash(fp, (uint64_t *)&db->xxh64);
    if (rc) goto cleanup;

    safe_strcpy(db->filepath, filepath, PATH_SIZE);

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

    if (xsql_reset(st)) return efail("reset");

    if (xsql_bind_i64(st, 0, db.xxh64)) return efail("bind");
    if (xsql_bind_txt(st, 1, XSQL_TXT_OF(db, filepath))) return efail("bind");

    if (xsql_step(st) != RC_DONE) return efail("add db");
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
    if (xsql_reset(st)) return efail("reset");

    if (xsql_bind_i64(st, 0, by_value)) return efail("bind");

    enum rc rc = xsql_step(st);
    if (rc == RC_DONE) return RC_NOTFOUND;
    if (rc != RC_NEXT) return efail("get db");

    db->id = sqlite3_column_int64(st, 0);
    db->xxh64 = sqlite3_column_int64(st, 1);
    if (xsql_cpy_txt(st, 2, XSQL_TXT_OF(*db, filepath)))
        return efail("copy txt");

    if (xsql_step(st)) return efail("step");
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

void db_name(char *filename, char const *path)
{
    xfile_basename(filename, path);
    xfile_strip_ext(filename);
}
