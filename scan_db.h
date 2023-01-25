#ifndef SCAN_DB_H
#define SCAN_DB_H

#include "db_reader.h"
#include "protein_reader.h"
#include <stdio.h>

struct scan_db
{
  char filename[FILENAME_MAX];
  FILE *fp;
  struct db_reader reader;
  struct protein_reader protein;
};

void scan_db_init(struct scan_db *);
int scan_db_open(struct scan_db *, int nthreads);
void scan_db_close(struct scan_db *);

int scan_db_set_filename(struct scan_db *, char const *);

#endif
