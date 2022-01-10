#include "xsql.h"
#include "common/rc.h"
#include <assert.h>
#include "common/safe.h"
#include <sqlite3.h>
#include <stdlib.h>

int xsql_bind_dbl(struct sqlite3_stmt *stmt, int col, double val)
{
    assert(col >= 0);
    if (sqlite3_bind_double(stmt, col + 1, val)) return RC_FAIL;
    return RC_DONE;
}

int xsql_bind_i64(struct sqlite3_stmt *stmt, int col, int64_t val)
{
    assert(col >= 0);
    if (sqlite3_bind_int64(stmt, col + 1, val)) return RC_FAIL;
    return RC_DONE;
}

int xsql_bind_str(struct sqlite3_stmt *stmt, int col, char const *str)
{
    assert(col >= 0);
    if (sqlite3_bind_text(stmt, col + 1, str, -1, SQLITE_TRANSIENT))
        return RC_FAIL;
    return RC_DONE;
}

int xsql_bind_txt(struct sqlite3_stmt *stmt, int col, struct xsql_txt txt)
{
    assert(col >= 0);
    if (sqlite3_bind_text(stmt, col + 1, txt.str, (int)txt.len,
                          SQLITE_TRANSIENT))
        return RC_FAIL;
    return RC_DONE;
}

int xsql_cpy_txt(struct sqlite3_stmt *stmt, int col, struct xsql_txt txt)
{
    char const *str = (char const *)sqlite3_column_text(stmt, col);
    if (!str) return RC_FAIL;
    sqlite3_column_bytes(stmt, col);
    safe_strcpy((char *)txt.str, str, txt.len + 1);
    return RC_DONE;
}

int xsql_open(char const *filepath, struct sqlite3 **db)
{
    if (sqlite3_open(filepath, db)) return RC_FAIL;
    if (xsql_exec(*db, "PRAGMA foreign_keys = ON;", 0, 0))
    {
        sqlite3_close(*db);
        return RC_FAIL;
    }
    return RC_DONE;
}

int xsql_close(struct sqlite3 *db)
{
    if (sqlite3_close(db)) return RC_FAIL;
    return RC_DONE;
}

int xsql_exec(struct sqlite3 *db, char const *sql, xsql_callback cb,
              void *cb_arg)
{
    if (sqlite3_exec(db, sql, cb, cb_arg, 0)) return RC_FAIL;
    return RC_DONE;
}

int xsql_begin_transaction(struct sqlite3 *db)
{
    return xsql_exec(db, "BEGIN TRANSACTION;", 0, 0);
}

int xsql_end_transaction(struct sqlite3 *db)
{
    return xsql_exec(db, "END TRANSACTION;", 0, 0);
}

int xsql_rollback_transaction(struct sqlite3 *db)
{
    return xsql_exec(db, "ROLLBACK TRANSACTION;", 0, 0);
}

int xsql_prepare(struct sqlite3 *db, char const *sql,
                 struct sqlite3_stmt **stmt)
{
    if (sqlite3_prepare_v2(db, sql, -1, stmt, 0)) return RC_FAIL;
    return RC_DONE;
}

int xsql_reset(struct sqlite3_stmt *stmt)
{
    if (sqlite3_reset(stmt)) return RC_FAIL;
    return RC_DONE;
}

int xsql_step(struct sqlite3_stmt *stmt)
{
    int code = sqlite3_step(stmt);
    if (code == SQLITE_DONE) return RC_DONE;
    if (code == SQLITE_ROW) return RC_NEXT;
    return RC_FAIL;
}

int xsql_end_step(struct sqlite3_stmt *stmt)
{
    if (sqlite3_step(stmt) != SQLITE_DONE) return RC_FAIL;
    return RC_DONE;
}

int64_t xsql_last_id(struct sqlite3 *db)
{
    return sqlite3_last_insert_rowid(db);
}
