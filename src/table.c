#include "table.h"
#include "macros.h"
#include "xstrlcpy.h"
#include <assert.h>
#include <string.h>

static inline void fill_space(char *str, unsigned size)
{
    assert(size > 0);
    str[size] = '\0';

    char const *end = str + size;
    while (*str)
        str++;

    while (str < end)
    {
        *str = ' ';
        str++;
    }
}

void table_set_headers(struct table *t, char const *cons, char const *seq,
                       char const *pcodon, char const *pamino,
                       char const *state)
{
    safe_strcpy(t->consensus.hdr, cons, MEMBER_SIZE(t->consensus, hdr));
    safe_strcpy(t->sequence.hdr, seq, MEMBER_SIZE(t->sequence, hdr));
    safe_strcpy(t->post_codon.hdr, pcodon, MEMBER_SIZE(t->post_codon, hdr));
    safe_strcpy(t->post_amino.hdr, pamino, MEMBER_SIZE(t->post_amino, hdr));
    safe_strcpy(t->state.hdr, state, MEMBER_SIZE(t->state, hdr));

    fill_space(t->consensus.hdr, MEMBER_SIZE(t->consensus, hdr) - 1);
    fill_space(t->sequence.hdr, MEMBER_SIZE(t->sequence, hdr) - 1);
    fill_space(t->post_codon.hdr, MEMBER_SIZE(t->post_codon, hdr) - 1);
    fill_space(t->post_amino.hdr, MEMBER_SIZE(t->post_amino, hdr) - 1);
    fill_space(t->state.hdr, MEMBER_SIZE(t->state, hdr) - 1);
}

bool table_add(struct table *tbl, char cons, unsigned seq_size,
               char const seq[seq_size], char const post_codon[3],
               char post_amino, unsigned state_size,
               char const state[state_size])
{
    if (row_full(&tbl->sequence.row))
    {
        if (!table_flush(tbl)) return false;
    }
    row_add(&tbl->consensus.row, &cons, 1);
    row_add(&tbl->sequence.row, seq, seq_size);
    row_add(&tbl->post_codon.row, post_codon, 3);
    row_add(&tbl->post_amino.row, &post_amino, 1);
    row_add(&tbl->state.row, state, state_size);
    return true;
}

static inline bool flush(struct table_row *trow, FILE *restrict fd)
{
    if (EOF == fputs(trow->hdr, fd)) return false;
    if (EOF == fputc(' ', fd)) return false;
    return row_flush(&trow->row, fd);
}

bool table_flush(struct table *tbl)
{
    if (!flush(&tbl->consensus, tbl->fd)) return false;
    if (!flush(&tbl->sequence, tbl->fd)) return false;
    if (!flush(&tbl->post_codon, tbl->fd)) return false;
    if (!flush(&tbl->post_amino, tbl->fd)) return false;
    if (!flush(&tbl->state, tbl->fd)) return false;
    return fputc('\n', tbl->fd) != EOF;
}
