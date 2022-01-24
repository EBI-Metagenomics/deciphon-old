#ifndef ROW_H
#define ROW_H

#include <stdbool.h>
#include <stdio.h>

#define ROW_SIZE 256

/* Row definition
 *
 * `data` starts here
 * |
 * _cell_cell_cell_cell*
 * ||
 * |Print starts here
 * First character (space) is not printed
 *
 * * is the null character '\0'
 * _ is the space character ' '
 *
 * Each cell has `cell_size` characters.
 */

struct row
{
    unsigned pos;
    unsigned capacity;
    unsigned cell_size;
    char data[ROW_SIZE];
};

#define ROW_INIT(capacity, cell_size)                                          \
    (struct row)                                                               \
    {                                                                          \
        0, capacity, cell_size, { ' ', '\0' }                                  \
    }

static inline void row_reset(struct row *row)
{
    row->pos = 0;
    row->data[0] = ' ';
    row->data[1] = '\0';
}

void row_add(struct row *row, char const *src, unsigned size);

bool row_flush(struct row *row, FILE *fp);

static inline bool row_full(struct row const *row)
{
    return row->pos + row->cell_size + 2 > row->capacity;
}

#endif
