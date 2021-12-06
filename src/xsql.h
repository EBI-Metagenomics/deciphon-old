#ifndef XSQL_H
#define XSQL_H

#include "rc.h"
#include <inttypes.h>

struct array;
struct sqlite3_stmt;

enum rc xsql_bind_dbl(struct sqlite3_stmt *stmt, int col, double val);
enum rc xsql_bind_i64(struct sqlite3_stmt *stmt, int col, int64_t val);
enum rc xsql_bind_str(struct sqlite3_stmt *stmt, int col, char const *str);
enum rc xsql_bind_txt(struct sqlite3_stmt *stmt, int col, unsigned len,
                      char const *txt);

enum rc xsql_get_txt(struct sqlite3_stmt *stmt, int col, unsigned dst_size,
                     char *dst);

enum rc xsql_get_text_as_array(struct sqlite3_stmt *stmt, int col,
                               struct array **dst);

#endif
