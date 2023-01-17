#ifndef DB_READER_H
#define DB_READER_H

#include "cfg.h"
#include "entry_dist.h"
#include "lip/lip.h"
#include "protein.h"

struct db_reader
{
  unsigned nproteins;
  uint32_t *protein_sizes;
  struct lip_file file;

  struct imm_amino amino;
  struct imm_nuclt nuclt;
  struct imm_nuclt_code code;
  struct cfg cfg;
};

int db_reader_open(struct db_reader *db, FILE *fp);
void db_reader_close(struct db_reader *db);

int db_reader_unpack_magic_number(struct db_reader *);
int db_reader_unpack_float_size(struct db_reader *);
int db_reader_unpack_prot_sizes(struct db_reader *db);

#endif
