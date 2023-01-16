#include "db_writer.h"
#include "defer_return.h"
#include "fs.h"
#include "lite_pack/1darray/1darray.h"
#include "magic_number.h"
#include "rc.h"

static int pack_entry_dist(struct lip_file *file, enum entry_dist const *edist)
{
  if (!lip_write_cstr(file, "entry_dist")) return EFWRITE;
  if (!lip_write_int(file, *edist)) return EFWRITE;
  return 0;
}

static int pack_epsilon(struct lip_file *file, imm_float const *epsilon)
{
  if (!lip_write_cstr(file, "epsilon")) return EFWRITE;
  if (!lip_write_float(file, *epsilon)) return EFWRITE;
  return 0;
}

static int pack_nuclt(struct lip_file *file, struct imm_nuclt const *nuclt)
{
  if (!lip_write_cstr(file, "abc")) return EFWRITE;
  if (imm_abc_pack(&nuclt->super, file)) return EFWRITE;
  return 0;
}

static int pack_amino(struct lip_file *file, struct imm_amino const *amino)
{
  if (!lip_write_cstr(file, "amino")) return EFWRITE;
  if (imm_abc_pack(&amino->super, file)) return EFWRITE;
  return 0;
}

static int pack_edist_cb(struct lip_file *file, void const *arg)
{
  return pack_entry_dist(file, arg);
}

static int pack_eps_cb(struct lip_file *file, void const *arg)
{
  return pack_epsilon(file, arg);
}

static int pack_nuclt_cb(struct lip_file *file, void const *arg)
{
  return pack_nuclt(file, arg);
}

static int pack_amino_cb(struct lip_file *file, void const *arg)
{
  return pack_amino(file, arg);
}

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

typedef int (*pack_prof_func_t)(struct lip_file *file, void const *arg);
typedef int (*pack_header_item_func_t)(struct lip_file *file, void const *arg);

static int db_writer_pack_magic_number(struct db_writer *db)
{
  if (!lip_write_cstr(&db->tmp.header, "magic_number")) return EFWRITE;

  if (!lip_write_int(&db->tmp.header, MAGIC_NUMBER)) return EFWRITE;

  db->header_size++;
  return 0;
}

static int db_writer_pack_float_size(struct db_writer *db)
{
  if (!lip_write_cstr(&db->tmp.header, "float_size")) return EFWRITE;

  unsigned size = IMM_FLOAT_BYTES;
  assert(size == 4 || size == 8);
  if (!lip_write_int(&db->tmp.header, size)) return EFWRITE;

  db->header_size++;
  return 0;
}

static int db_writer_pack_prof(struct db_writer *db,
                               pack_prof_func_t pack_profile, void const *arg)
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

static int db_writer_pack_header(struct db_writer *db,
                                 pack_header_item_func_t pack_header_item,
                                 void const *arg)
{
  db->header_size++;
  return pack_header_item(&db->tmp.header, arg);
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

int db_writer_open(struct db_writer *db, FILE *fp,
                   struct imm_amino const *amino, struct imm_nuclt const *nuclt,
                   struct cfg cfg)
{
  int rc = 0;

  db->nprofiles = 0;
  db->header_size = 0;
  lip_file_init(&db->file, fp);
  if ((rc = create_tempfiles(db))) return rc;

  db->amino = *amino;
  db->nuclt = *nuclt;
  imm_nuclt_code_init(&db->code, &db->nuclt);
  db->cfg = cfg;

  imm_float const *epsilon = &db->cfg.eps;
  enum entry_dist const *edist = &db->cfg.edist;

  struct imm_nuclt const *n = &db->nuclt;
  struct imm_amino const *a = &db->amino;
  if ((rc = db_writer_pack_magic_number(db))) defer_return(rc);
  if ((rc = db_writer_pack_float_size(db))) defer_return(rc);
  if ((rc = db_writer_pack_header(db, pack_edist_cb, edist))) defer_return(rc);
  if ((rc = db_writer_pack_header(db, pack_eps_cb, epsilon))) defer_return(rc);
  if ((rc = db_writer_pack_header(db, pack_nuclt_cb, n))) defer_return(rc);
  if ((rc = db_writer_pack_header(db, pack_amino_cb, a))) defer_return(rc);

  return rc;

defer:
  db_writer_close(db, false);
  return rc;
}

static int pack_profile(struct lip_file *file, void const *prof)
{
  return protein_pack(prof, file);
}

int db_writer_pack(struct db_writer *db, struct protein const *profile)
{
  return db_writer_pack_prof(db, pack_profile, profile);
}
