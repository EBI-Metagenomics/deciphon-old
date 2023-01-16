#ifndef DB_READER_H
#define DB_READER_H

#include "lite_pack/lite_pack.h"

struct db_reader
{
  unsigned nprofiles;
  uint32_t *profile_sizes;
  struct lip_file file;
};

int db_reader_open(struct db_reader *db, FILE *fp);
void db_reader_close(struct db_reader *db);

int db_reader_unpack_magic_number(struct db_reader *);
int db_reader_unpack_float_size(struct db_reader *);
int db_reader_unpack_prof_sizes(struct db_reader *db);

#endif
