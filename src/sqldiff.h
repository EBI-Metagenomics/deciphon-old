#ifndef SQLDIFF_H
#define SQLDIFF_H

#include <stdbool.h>

enum rc sqldiff_compare(char const *db0, char const *db1, bool *equal);

#endif
