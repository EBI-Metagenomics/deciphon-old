#ifndef XSQL_H
#define XSQL_H

#include "rc.h"

struct array;
struct sqlite3_stmt;

enum dcp_rc xsql_get_text(struct sqlite3_stmt *stmt, int col, unsigned dst_size,
                          char *dst);

enum dcp_rc xsql_get_text_as_array(struct sqlite3_stmt *stmt, int col,
                                   struct array **dst);

#endif
