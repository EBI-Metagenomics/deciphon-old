#ifndef DB_READER_H
#define DB_READER_H

#include "entry_dist.h"
#include "imm/imm.h"
#include "lip/lip.h"
#include "protein.h"

struct db_reader
{
  int nproteins;
  uint32_t *protein_sizes;
  struct lip_file file;

  struct imm_amino amino;
  struct imm_nuclt nuclt;
  struct imm_nuclt_code code;
  enum entry_dist entry_dist;
  float epsilon;
};

void db_reader_init(struct db_reader *);
int db_reader_open(struct db_reader *, FILE *);
void db_reader_close(struct db_reader *);

int db_reader_unpack_magic_number(struct db_reader *);
int db_reader_unpack_float_size(struct db_reader *);
int db_reader_unpack_prot_sizes(struct db_reader *);

#endif
