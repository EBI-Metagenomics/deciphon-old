#ifndef SCAN_PRODFILE_READER_H
#define SCAN_PRODFILE_READER_H

#include "scan/prod.h"

struct prodfile_reader;

int prodfile_reader_new(struct prodfile_reader **);
int prodfile_reader_open(struct prodfile_reader *, char const *file);
int prodfile_reader_next(struct prodfile_reader *, struct prod *);
int prodfile_reader_close(struct prodfile_reader *);
void prodfile_reader_del(struct prodfile_reader *);

#endif
