#include "xsql.h"
#include "array.h"
#include "logger.h"
#include <assert.h>
#include <safe.h>
#include <sqlite3.h>
#include <stdlib.h>

enum dcp_rc xsql_get_text(struct sqlite3_stmt *stmt, int col, unsigned dst_size,
                          char *dst)
{
    char const *str = (char const *)sqlite3_column_text(stmt, col);
    if (!str) return error(DCP_OUTOFMEM, "failed to fetch sqlite text");
    sqlite3_column_bytes(stmt, col);
    assert((unsigned)len < dst_size);
    safe_strcpy(dst, str, dst_size);
    return DCP_DONE;
}

enum dcp_rc xsql_get_text_as_array(struct sqlite3_stmt *stmt, int col,
                                   struct array **dst)
{
    char const *str = (char const *)sqlite3_column_text(stmt, col);
    if (!str) return error(DCP_OUTOFMEM, "failed to fetch sqlite text");
    int len = sqlite3_column_bytes(stmt, col);
    struct array *arr = array_put(*dst, str, (size_t)(len + 1));
    if (!arr) return error(DCP_OUTOFMEM, "failed to copy sqlite text");
    *dst = arr;
    return DCP_DONE;
}
