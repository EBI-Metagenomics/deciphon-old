#include "db_reader.h"
#include "compiler.h"
#include "db_types.h"
#include "deciphon_limits.h"
#include "expect.h"
#include "imm/imm.h"
#include "lite_pack/1darray/1darray.h"
#include <stdlib.h>

int db_reader_open(struct db_reader *db, FILE *fp)
{
  db->nprofiles = 0;
  db->profile_sizes = 0;
  db->prof_typeid = PROF_NULL;
  lip_file_init(&db->file, fp);
  return 0;
}

void db_reader_close(struct db_reader *db)
{
  if (db->profile_sizes) free(db->profile_sizes);
}

int db_reader_unpack_magic_number(struct db_reader *db)
{
  int rc = 0;

  if ((rc = expect_map_key(&db->file, "magic_number"))) return rc;

  unsigned number = 0;
  if (!lip_read_int(&db->file, &number)) return EFREAD;

  return number != MAGIC_NUMBER ? EFDATA : 0;
}

int db_reader_unpack_prof_typeid(struct db_reader *db, enum prof_typeid typeid)
{
  int rc = 0;

  if ((rc = expect_map_key(&db->file, "profile_typeid"))) return rc;

  if (!lip_read_int(&db->file, &db->prof_typeid)) return EFREAD;

  if (db->prof_typeid != typeid) return EFDATA;

  return 0;
}

int db_reader_unpack_float_size(struct db_reader *db)
{
  int rc = 0;
  if ((rc = expect_map_key(&db->file, "float_size"))) return rc;

  unsigned size = 0;
  if (!lip_read_int(&db->file, &size)) return EFREAD;

  return size != IMM_FLOAT_BYTES ? EFDATA : 0;
}

static int unpack_header_profile_sizes(struct db_reader *db)
{
  enum lip_1darray_type type = 0;

  if (!lip_read_1darray_size_type(&db->file, &db->nprofiles, &type))
    return EFREAD;

  if (type != LIP_1DARRAY_UINT32) return EFDATA;

  db->profile_sizes = malloc(sizeof(*db->profile_sizes) * db->nprofiles);
  if (!db->profile_sizes) return ENOMEM;

  if (!lip_read_1darray_u32_data(&db->file, db->nprofiles, db->profile_sizes))
  {
    free(db->profile_sizes);
    db->profile_sizes = 0;
    return EFREAD;
  }

  return 0;
}

int db_reader_unpack_prof_sizes(struct db_reader *db)
{
  int rc = 0;
  if ((rc = expect_map_key(&db->file, "profile_sizes"))) return rc;
  return unpack_header_profile_sizes(db);
}
