#ifndef TABLE_H
#define TABLE_H

#include "row.h"

#define TABLE_HEADER_SIZE 16

struct table_row
{
    char hdr[TABLE_HEADER_SIZE];
    struct row row;
};

#define TABLE_ROW_INIT(capacity, cell_size)                                    \
    {                                                                          \
        {0}, ROW_INIT(capacity, cell_size)                                     \
    }

struct table
{
    FILE *fd;
    struct table_row consensus;
    /* struct row difference; */
    struct table_row sequence;
    struct table_row post_codon;
    struct table_row post_amino;
    struct table_row state;
    /* consensus structure SBCEEEEEEESSSEEEEEEE-CSSSSST */
    /* consensus           aPsnlsvtevtstsltvsWtppedgngp */
    /* difference          aP +++ ++ l v+W+p + ngpi+gY+ */
    /* sequence            APVIEHLMGLDDSHLAVHWHPGRFTNGP */
};

#define TABLE_INIT(fd, capacity, cell_size)                                    \
    {                                                                          \
        fd, TABLE_ROW_INIT(capacity, cell_size),                               \
            TABLE_ROW_INIT(capacity, cell_size),                               \
            TABLE_ROW_INIT(capacity, cell_size),                               \
            TABLE_ROW_INIT(capacity, cell_size),                               \
            TABLE_ROW_INIT(capacity, cell_size)                                \
    }

static inline void table_setup(struct table *tbl, FILE *fd) { tbl->fd = fd; }

void table_set_headers(struct table *tbl, char const *cons, char const *seq,
                       char const *pcodon, char const *pamino,
                       char const *state);

bool table_add(struct table *tbl, char cons, unsigned seq_size,
               char const seq[seq_size], char const post_codon[3],
               char post_amino, unsigned state_size,
               char const state[state_size]);

bool table_flush(struct table *tbl);

#if 0
== domain 2  score: 41.9 bits; conditional E-value: 5.1e-15
                SBCEEEEEEESSSEEEEEEE-CSSSSSTECEEEEEEEETTSSSTEEEEEEESTCSEEEEESSSTTEEEEEEEEEEETTEEEE CS
        fn3   2 aPsnlsvtevtstsltvsWtppedgngpitgYeveyrpknegeewneitvpgtttsvtltgLkpgteYevrVqavngggegp 83
                aP +++ ++ l v+W+p + ngpi+gY++++++++++ + e+ vp s+++++L++gt+Y++ + +n++gegp
7LESS_DROME 440 APVIEHLMGLDDSHLAVHWHPGRFTNGPIEGYRLRLSSSEGNA-TSEQLVPAGRGSYIFSQLQAGTNYTLALSMINKQGEGP 520
                77778899999****************************9997.***********************************997 PP
#endif

#endif
