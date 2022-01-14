#include "row.h"
#include <assert.h>
#include <string.h>

void row_add(struct row *row, char const *src, unsigned size)
{
    assert(size <= row->cell_size);
    row->data[row->pos++] = ' ';

    memcpy(row->data + row->pos, src, size);
    row->pos += size;

    for (unsigned i = 0; i < (unsigned)(row->cell_size - size); ++i)
        row->data[row->pos++] = ' ';

    row->data[row->pos] = '\0';
}

bool row_flush(struct row *row, FILE *restrict fd)
{
    if (EOF == fputs(row->data + 1, fd)) return false;
    if (EOF == fputc('\n', fd)) return false;
    row_reset(row);
    return true;
}
