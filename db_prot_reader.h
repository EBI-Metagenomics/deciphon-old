#ifndef DB_PROT_READER_H
#define DB_PROT_READER_H

#include "db_reader.h"
#include "entry_dist.h"
#include "prot_cfg.h"
#include "prot_prof.h"

struct prot_db_reader
{
  struct db_reader super;
  struct imm_amino amino;
  struct imm_nuclt nuclt;
  struct imm_nuclt_code code;
  struct prot_cfg cfg;
};

int prot_db_reader_open(struct prot_db_reader *db, FILE *fp);

#endif
