#include "db_writer.h"
#include "deciphon/errno.h"
#include "defer_return.h"
#include "fs.h"
#include "lip/1darray/1darray.h"
#include "magic_number.h"

static int pack_entry_dist(struct lip_file *file, enum entry_dist const *edist)
{
  if (!lip_write_cstr(file, "entry_dist")) return DCP_EFWRITE;
  if (!lip_write_int(file, *edist)) return DCP_EFWRITE;
  return 0;
}

static int pack_epsilon(struct lip_file *file, float const *epsilon)
{
  if (!lip_write_cstr(file, "epsilon")) return DCP_EFWRITE;
  if (!lip_write_float(file, *epsilon)) return DCP_EFWRITE;
  return 0;
}

static int pack_nuclt(struct lip_file *file, struct imm_nuclt const *nuclt)
{
  if (!lip_write_cstr(file, "abc")) return DCP_EFWRITE;
  if (imm_abc_pack(&nuclt->super, file)) return DCP_EFWRITE;
  return 0;
}

static int pack_amino(struct lip_file *file, struct imm_amino const *amino)
{
  if (!lip_write_cstr(file, "amino")) return DCP_EFWRITE;
  if (imm_abc_pack(&amino->super, file)) return DCP_EFWRITE;
  return 0;
}

static void destroy_tempfiles(struct db_writer *db)
{
  if (db->tmp.header.fp) fclose(db->tmp.header.fp);
  if (db->tmp.prot_sizes.fp) fclose(db->tmp.prot_sizes.fp);
  if (db->tmp.proteins.fp) fclose(db->tmp.proteins.fp);
}

static int create_tempfiles(struct db_writer *db)
{
  lip_file_init(&db->tmp.header, tmpfile());
  lip_file_init(&db->tmp.prot_sizes, tmpfile());
  lip_file_init(&db->tmp.proteins, tmpfile());

  int rc = 0;
  if (!db->tmp.header.fp || !db->tmp.prot_sizes.fp || !db->tmp.proteins.fp)
    defer_return(DCP_EOPENTMP);

  return rc;

defer:
  destroy_tempfiles(db);
  return rc;
}

static int db_writer_pack_magic_number(struct db_writer *db)
{
  if (!lip_write_cstr(&db->tmp.header, "magic_number")) return DCP_EFWRITE;

  if (!lip_write_int(&db->tmp.header, MAGIC_NUMBER)) return DCP_EFWRITE;

  db->header_size++;
  return 0;
}

static int db_writer_pack_float_size(struct db_writer *db)
{
  if (!lip_write_cstr(&db->tmp.header, "float_size")) return DCP_EFWRITE;

  // unsigned size = IMM_FLOAT_BYTES;
  unsigned size = 4;
  assert(size == 4 || size == 8);
  if (!lip_write_int(&db->tmp.header, size)) return DCP_EFWRITE;

  db->header_size++;
  return 0;
}

static int pack_protein(struct lip_file *file, void const *protein)
{
  return protein_pack(protein, file);
}

static int db_writer_pack_prof(struct db_writer *db, void const *arg)
{
  int rc = 0;

  long start = 0;
  if ((rc = fs_tell(lip_file_ptr(&db->tmp.proteins), &start))) return rc;

  if ((rc = pack_protein(&db->tmp.proteins, arg))) return rc;

  long end = 0;
  if ((rc = fs_tell(lip_file_ptr(&db->tmp.proteins), &end))) return rc;

  if ((end - start) > UINT_MAX) return DCP_ELARGEPROTEIN;

  unsigned prot_size = (unsigned)(end - start);
  if (!lip_write_int(&db->tmp.prot_sizes, prot_size)) return DCP_EFWRITE;

  db->nproteins++;
  return rc;
}

static int pack_header_prot_sizes(struct db_writer *db)
{
  enum lip_1darray_type type = LIP_1DARRAY_UINT32;

  if (!lip_write_1darray_size_type(&db->file, db->nproteins, type))
    return DCP_EFWRITE;

  rewind(lip_file_ptr(&db->tmp.prot_sizes));

  unsigned size = 0;
  while (lip_read_int(&db->tmp.prot_sizes, &size))
    lip_write_1darray_u32_item(&db->file, size);

  if (!feof(lip_file_ptr(&db->tmp.prot_sizes))) return DCP_EFWRITE;

  return 0;
}

static int pack_header(struct db_writer *db)
{
  struct lip_file *file = &db->file;
  if (!lip_write_cstr(file, "header")) return DCP_EFWRITE;

  if (!lip_write_map_size(file, db->header_size + 1)) return DCP_EFWRITE;

  rewind(lip_file_ptr(&db->tmp.header));
  int rc = fs_copy(lip_file_ptr(file), lip_file_ptr(&db->tmp.header));
  if (rc) return rc;

  if (!lip_write_cstr(file, "protein_sizes")) return DCP_EFWRITE;
  return pack_header_prot_sizes(db);
}

static int pack_proteins(struct db_writer *db)
{
  if (!lip_write_cstr(&db->file, "proteins")) return DCP_EFWRITE;

  if (!lip_write_array_size(&db->file, db->nproteins)) return DCP_EFWRITE;

  rewind(lip_file_ptr(&db->tmp.proteins));
  return fs_copy(lip_file_ptr(&db->file), lip_file_ptr(&db->tmp.proteins));
}

int db_writer_close(struct db_writer *db)
{
  int rc = 0;
  if (!lip_write_map_size(&db->file, 2)) defer_return(DCP_EFWRITE);

  if ((rc = pack_header(db))) defer_return(rc);

  if ((rc = pack_proteins(db))) defer_return(rc);

  return rc;

defer:
  destroy_tempfiles(db);
  return rc;
}

int db_writer_open(struct db_writer *db, FILE *fp,
                   struct imm_amino const *amino, struct imm_nuclt const *nuclt,
                   enum entry_dist entry_dist, float epsilon)
{
  int rc = 0;

  db->nproteins = 0;
  db->header_size = 0;
  lip_file_init(&db->file, fp);
  if ((rc = create_tempfiles(db))) return rc;

  db->amino = *amino;
  db->nuclt = *nuclt;
  imm_nuclt_code_init(&db->code, &db->nuclt);
  db->entry_dist = entry_dist;
  db->epsilon = epsilon;

  struct imm_nuclt const *n = &db->nuclt;
  struct imm_amino const *a = &db->amino;
  if ((rc = db_writer_pack_magic_number(db))) defer_return(rc);
  if ((rc = db_writer_pack_float_size(db))) defer_return(rc);
  if ((rc = pack_entry_dist(&db->tmp.header, &entry_dist))) defer_return(rc);
  if ((rc = pack_epsilon(&db->tmp.header, &epsilon))) defer_return(rc);
  if ((rc = pack_nuclt(&db->tmp.header, n))) defer_return(rc);
  if ((rc = pack_amino(&db->tmp.header, a))) defer_return(rc);
  db->header_size += 4;

  return rc;

defer:
  db_writer_close(db);
  return rc;
}

int db_writer_pack(struct db_writer *db, struct protein const *protein)
{
  return db_writer_pack_prof(db, protein);
}
