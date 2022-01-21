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

static enum rc init_db(struct sched_db *db, char const *filename)
{
    FILE *fp = fopen(filename, "rb");
    if (!fp) return eio("fopen");

    enum rc rc = xfile_hash(fp, (uint64_t *)&db->xxh64);
    if (rc) goto cleanup;

    safe_strcpy(db->filename, filename, ARRAY_SIZE_OF(*db, filename));

cleanup:
    fclose(fp);
    return rc;
}

static enum rc select_db_i64(struct sched_db *db, int64_t by_value,
                             enum stmt select_stmt)
{
    struct sqlite3_stmt *st = stmt[select_stmt];
    if (xsql_reset(st)) return efail("reset");

    if (xsql_bind_i64(st, 0, by_value)) return efail("bind");

    enum rc rc = xsql_step(st);
    if (rc == RC_DONE) return RC_NOTFOUND;
    if (rc != RC_NEXT) return efail("get db");

    db->id = sqlite3_column_int64(st, 0);
    db->xxh64 = sqlite3_column_int64(st, 1);
    if (xsql_cpy_txt(st, 2, XSQL_TXT_OF(*db, filename)))
        return efail("copy txt");

    if (xsql_step(st)) return efail("step");
    return RC_DONE;
}

static enum rc select_db_str(struct sched_db *db, char const *by_value,
                             enum stmt select_stmt)
{
    struct sqlite3_stmt *st = stmt[select_stmt];
    printf("Ponto 1\n");
    if (xsql_reset(st)) return efail("reset");

    printf("Ponto 2\n");
    if (xsql_bind_str(st, 0, by_value)) return efail("bind");

    printf("Ponto 3\n");
    enum rc rc = xsql_step(st);
    if (rc == RC_DONE) return RC_NOTFOUND;
    if (rc != RC_NEXT) return efail("get db");

    printf("Ponto 4\n");
    db->id = sqlite3_column_int64(st, 0);
    db->xxh64 = sqlite3_column_int64(st, 1);
    if (xsql_cpy_txt(st, 2, XSQL_TXT_OF(*db, filename)))
        return efail("copy txt");

    printf("Ponto 5\n");
    if (xsql_step(st)) return efail("step");
    printf("Ponto 6\n");
    return RC_DONE;
}

static enum rc add_db(char const *filename, struct sched_db *db)
{
    struct sqlite3_stmt *st = stmt[DB_INSERT];

    enum rc rc = init_db(db, filename);
    if (rc) return rc;

    if (xsql_reset(st)) return efail("reset");

    if (xsql_bind_i64(st, 0, db->xxh64)) return efail("bind");
    if (xsql_bind_str(st, 1, filename)) return efail("bind");

    if (xsql_step(st) != RC_DONE) return efail("add db");
    db->id = xsql_last_id(sched);
    return RC_DONE;
}

enum rc sched_db_add(struct sched_db *db, char const *filename)
{
    struct sched_db tmp = {0};
    enum rc rc = select_db_str(&tmp, filename, DB_SELECT_BY_FILENAME);

    if (rc == RC_DONE)
        return error(RC_EFAIL, "db with same filename already exist");

    if (rc == RC_NOTFOUND) return add_db(filename, db);

    return rc;
}

enum rc db_has(char const *filename, struct sched_db *db)
{
    enum rc rc = init_db(db, filename);
    if (rc) return rc;
    return db_get_by_xxh64(db, db->xxh64);
}

enum rc db_get_by_xxh64(struct sched_db *db, int64_t xxh64)
{
    return select_db_i64(db, xxh64, DB_SELECT_BY_XXH64);
}

enum rc db_hash(char const *filename, int64_t *xxh64)
{
    struct sched_db db = {0};
    enum rc rc = init_db(&db, filename);
    *xxh64 = db.xxh64;
    return rc;
}

void sched_db_init(struct sched_db *db)
{
    db->id = 0;
    db->xxh64 = 0;
    db->filename[0] = 0;
}

enum rc sched_db_get(struct sched_db *db)
{
    if (db->id) return select_db_i64(db, db->id, DB_SELECT_BY_ID);
    if (db->xxh64) return select_db_i64(db, db->xxh64, DB_SELECT_BY_XXH64);
    return select_db_str(db, db->filename, DB_SELECT_BY_FILENAME);
}
