#ifndef SQLDIFF_H
#define SQLDIFF_H

#include <stdbool.h>

enum dcp_rc;

enum dcp_rc sqldiff_compare(char const *db0, char const *db1, bool *equal);

#endif
