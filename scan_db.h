#ifndef SCAN_DB_H
#define SCAN_DB_H

#include "db_reader.h"
#include "protein_reader.h"
#include <stdio.h>

struct scan_db
{
  char filename[FILENAME_MAX];
  FILE *fp;
  struct db_reader db;
  struct protein_reader rdr;
};

struct imm_abc;

void scan_db_init(struct scan_db *);
int scan_db_open(struct scan_db *, int nthreads);
void scan_db_close(struct scan_db *);

int scan_db_set_filename(struct scan_db *, char const *);
struct protein_reader *scan_db_reader(struct scan_db *);
struct imm_abc const *scan_db_abc(struct scan_db const *);

#endif
