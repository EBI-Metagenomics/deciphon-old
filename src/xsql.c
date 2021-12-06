#include "xsql.h"
#include "array.h"
#include "logger.h"
#include <assert.h>
#include <safe.h>
#include <sqlite3.h>
#include <stdlib.h>

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

enum rc xsql_bind_txt(struct sqlite3_stmt *stmt, int col, unsigned len,
                      char const *txt)
{
    assert(col >= 0);
    if (sqlite3_bind_text(stmt, col + 1, txt, (int)len, SQLITE_TRANSIENT))
        return error(RC_FAIL, "failed to bind text");
    return RC_DONE;
}

enum rc xsql_get_txt(struct sqlite3_stmt *stmt, int col, unsigned dst_size,
                     char *dst)
{
    char const *str = (char const *)sqlite3_column_text(stmt, col);
    if (!str) return error(RC_OUTOFMEM, "failed to fetch sqlite text");
    sqlite3_column_bytes(stmt, col);
    safe_strcpy(dst, str, dst_size);
    return RC_DONE;
}

enum rc xsql_get_text_as_array(struct sqlite3_stmt *stmt, int col,
                               struct array **dst)
{
    char const *str = (char const *)sqlite3_column_text(stmt, col);
    if (!str) return error(RC_OUTOFMEM, "failed to fetch sqlite text");
    int len = sqlite3_column_bytes(stmt, col);
    struct array *arr = array_put(*dst, str, (size_t)(len + 1));
    if (!arr) return error(RC_OUTOFMEM, "failed to copy sqlite text");
    *dst = arr;
    return RC_DONE;
}
