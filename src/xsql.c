#include "xsql.h"
#include "array.h"
#include "logger.h"
#include <assert.h>
#include <safe.h>
#include <sqlite3.h>
#include <stdlib.h>

enum rc xsql_txt_as_array(struct xsql_txt const *txt, struct array **arr)
{
    *arr = array_put(*arr, txt->str, (size_t)(txt->len + 1));
    if (!*arr) return error(RC_OUTOFMEM, "failed to convert txt to array");
    return RC_DONE;
}

enum rc xsql_bind_dbl(struct sqlite3_stmt *stmt, int col, double val)
{
    assert(col >= 0);
    if (sqlite3_bind_double(stmt, col + 1, val))
        return error(RC_FAIL, "failed to bind double");
    return RC_DONE;
}

enum rc xsql_bind_i64(struct sqlite3_stmt *stmt, int col, int64_t val)
{
    assert(col >= 0);
    if (sqlite3_bind_int64(stmt, col + 1, val))
        return error(RC_FAIL, "failed to bind int64");
    return RC_DONE;
}

enum rc xsql_bind_str(struct sqlite3_stmt *stmt, int col, char const *str)
{
    assert(col >= 0);
    if (sqlite3_bind_text(stmt, col + 1, str, -1, SQLITE_TRANSIENT))
        return error(RC_FAIL, "failed to bind string");
    return RC_DONE;
}

enum rc xsql_bind_txt(struct sqlite3_stmt *stmt, int col, struct xsql_txt txt)
{
    assert(col >= 0);
    if (sqlite3_bind_text(stmt, col + 1, txt.str, (int)txt.len,
                          SQLITE_TRANSIENT))
        return error(RC_FAIL, "failed to bind text");
    return RC_DONE;
}

enum rc xsql_get_txt(struct sqlite3_stmt *stmt, int col, struct xsql_txt *txt)
{
    txt->str = (char const *)sqlite3_column_text(stmt, col);
    if (!txt->str) return error(RC_OUTOFMEM, "failed to fetch sqlite text");
    int ilen = sqlite3_column_bytes(stmt, col);
    assert(ilen > 0);
    txt->len = (unsigned)ilen;
    return RC_DONE;
}

enum rc xsql_cpy_txt(struct sqlite3_stmt *stmt, int col, struct xsql_txt txt)
{
    char const *str = (char const *)sqlite3_column_text(stmt, col);
    if (!str) return error(RC_OUTOFMEM, "failed to fetch sqlite text");
    sqlite3_column_bytes(stmt, col);
    safe_strcpy((char *)txt.str, str, txt.len + 1);
    return RC_DONE;
}
