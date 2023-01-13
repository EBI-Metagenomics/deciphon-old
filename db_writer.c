#include "db_writer.h"
#include "compiler.h"
#include "db_types.h"
#include "deciphon_limits.h"
#include "defer_return.h"
#include "fs.h"
#include "imm/imm.h"
#include "lite_pack/1darray/1darray.h"
#include "rc.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

static void destroy_tempfiles(struct db_writer *db)
{
  if (db->tmp.header.fp) fclose(db->tmp.header.fp);
  if (db->tmp.prof_sizes.fp) fclose(db->tmp.prof_sizes.fp);
  if (db->tmp.profiles.fp) fclose(db->tmp.profiles.fp);
}

static int create_tempfiles(struct db_writer *db)
{
  lip_file_init(&db->tmp.header, tmpfile());
  lip_file_init(&db->tmp.prof_sizes, tmpfile());
  lip_file_init(&db->tmp.profiles, tmpfile());

  int rc = 0;
  if (!db->tmp.header.fp || !db->tmp.prof_sizes.fp || !db->tmp.profiles.fp)
    defer_return(EOPENTMP);

  return rc;

defer:
  destroy_tempfiles(db);
  return rc;
}

int db_writer_open(struct db_writer *db, FILE *fp)
{
  db->nprofiles = 0;
  db->header_size = 0;
  lip_file_init(&db->file, fp);
  return create_tempfiles(db);
}

static int pack_header_prof_sizes(struct db_writer *db)
{
  enum lip_1darray_type type = LIP_1DARRAY_UINT32;

  if (!lip_write_1darray_size_type(&db->file, db->nprofiles, type))
    return EFWRITE;

  rewind(lip_file_ptr(&db->tmp.prof_sizes));

  unsigned size = 0;
  while (lip_read_int(&db->tmp.prof_sizes, &size))
    lip_write_1darray_u32_item(&db->file, size);

  if (!feof(lip_file_ptr(&db->tmp.prof_sizes))) return EFWRITE;

  return 0;
}

static int pack_header(struct db_writer *db)
{
  struct lip_file *file = &db->file;
  if (!lip_write_cstr(file, "header")) return EFWRITE;

  if (!lip_write_map_size(file, db->header_size + 1)) return EFWRITE;

  rewind(lip_file_ptr(&db->tmp.header));
  int rc = fs_copy(lip_file_ptr(file), lip_file_ptr(&db->tmp.header));
  if (rc) return rc;

  if (!lip_write_cstr(file, "profile_sizes")) return EFWRITE;
  return pack_header_prof_sizes(db);
}

static int pack_profiles(struct db_writer *db)
{
  if (!lip_write_cstr(&db->file, "profiles")) return EFWRITE;

  if (!lip_write_array_size(&db->file, db->nprofiles)) return EFWRITE;

  rewind(lip_file_ptr(&db->tmp.profiles));
  return fs_copy(lip_file_ptr(&db->file), lip_file_ptr(&db->tmp.profiles));
}

int db_writer_close(struct db_writer *db, bool successfully)
{
  if (!successfully)
  {
    destroy_tempfiles(db);
    return 0;
  }

  int rc = 0;
  if (!lip_write_map_size(&db->file, 2)) defer_return(EFWRITE);

  if ((rc = pack_header(db))) defer_return(rc);

  if ((rc = pack_profiles(db))) defer_return(rc);

  return rc;

defer:
  destroy_tempfiles(db);
  return rc;
}

int db_writer_pack_magic_number(struct db_writer *db)
{
  if (!lip_write_cstr(&db->tmp.header, "magic_number")) return EFWRITE;

  if (!lip_write_int(&db->tmp.header, MAGIC_NUMBER)) return EFWRITE;

  db->header_size++;
  return 0;
}

int db_writer_pack_prof_typeid(struct db_writer *db, int prof_typeid)
{
  if (!lip_write_cstr(&db->tmp.header, "profile_typeid")) return EFWRITE;

  if (!lip_write_int(&db->tmp.header, prof_typeid)) return EFWRITE;

  db->header_size++;
  return 0;
}

int db_writer_pack_float_size(struct db_writer *db)
{
  if (!lip_write_cstr(&db->tmp.header, "float_size")) return EFWRITE;

  unsigned size = IMM_FLOAT_BYTES;
  assert(size == 4 || size == 8);
  if (!lip_write_int(&db->tmp.header, size)) return EFWRITE;

  db->header_size++;
  return 0;
}

int db_writer_pack_prof(struct db_writer *db, pack_prof_func_t pack_profile,
                        void const *arg)
{
  int rc = 0;

  long start = 0;
  if ((rc = fs_tell(lip_file_ptr(&db->tmp.profiles), &start))) return rc;

  if ((rc = pack_profile(&db->tmp.profiles, arg))) return rc;

  long end = 0;
  if ((rc = fs_tell(lip_file_ptr(&db->tmp.profiles), &end))) return rc;

  if ((end - start) > UINT_MAX) return ELARGEPROFILE;

  unsigned prof_size = (unsigned)(end - start);
  if (!lip_write_int(&db->tmp.prof_sizes, prof_size)) return EFWRITE;

  db->nprofiles++;
  return rc;
}

int db_writer_pack_header(struct db_writer *db,
                          pack_header_item_func_t pack_header_item,
                          void const *arg)
{
  db->header_size++;
  return pack_header_item(&db->tmp.header, arg);
}
