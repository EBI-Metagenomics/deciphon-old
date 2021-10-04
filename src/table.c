#include "table.h"

bool table_add(struct table *tbl, unsigned seq_size, char const seq[seq_size],
               char cons, unsigned state_size, char const state[state_size])
{
    if (row_full(&tbl->sequence))
    {
        if (!table_flush(tbl)) return false;
    }
    row_add(&tbl->sequence, seq, seq_size);
    row_add(&tbl->consensus, &cons, 1);
    row_add(&tbl->state, state, state_size);
    return true;
}

bool table_flush(struct table *tbl)
{

    bool ok = row_flush(&tbl->sequence, tbl->fd);
    ok = ok && row_flush(&tbl->consensus, tbl->fd);
    ok = ok && row_flush(&tbl->state, tbl->fd);
    return ok && fputc('\n', tbl->fd) != EOF;
}
