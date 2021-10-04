#ifndef TABLE_H
#define TABLE_H

#include "row.h"

struct table
{
    FILE *fd;
    struct row sequence;
    struct row consensus;
    struct row state;
};

#define TABLE_INIT(fd, capacity, cell_size)                                    \
    {                                                                          \
        fd, ROW_INIT(capacity, cell_size), ROW_INIT(capacity, cell_size),      \
            ROW_INIT(capacity, cell_size)                                      \
    }

static inline void table_setup(struct table *tbl, FILE *fd) { tbl->fd = fd; }

bool table_add(struct table *tbl, unsigned seq_size, char const seq[seq_size],
               char cons, unsigned state_size, char const state[state_size]);

bool table_flush(struct table *tbl);

#endif
