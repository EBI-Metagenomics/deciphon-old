#ifndef XSQL_H
#define XSQL_H

#include "compiler.h"
#include "rc.h"
#include <inttypes.h>

struct array;
struct sqlite3_stmt;

struct xsql_txt
{
    unsigned len;
    char const *str;
};

#define XSQL_TXT_OF(var, member)                                               \
    (struct xsql_txt) { ARRAY_SIZE_OF((var), member), (var).member }

enum rc xsql_txt_as_array(struct xsql_txt const *txt, struct array **arr);

enum rc xsql_bind_dbl(struct sqlite3_stmt *stmt, int col, double val);
enum rc xsql_bind_i64(struct sqlite3_stmt *stmt, int col, int64_t val);
enum rc xsql_bind_str(struct sqlite3_stmt *stmt, int col, char const *str);
enum rc xsql_bind_txt(struct sqlite3_stmt *stmt, int col, struct xsql_txt txt);

enum rc xsql_get_txt(struct sqlite3_stmt *stmt, int col, struct xsql_txt *txt);
enum rc xsql_cpy_txt(struct sqlite3_stmt *stmt, int col, struct xsql_txt txt);

#endif
