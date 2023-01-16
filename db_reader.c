#include "db_reader.h"
#include "defer_return.h"
#include "expect.h"
#include "imm/imm.h"
#include "lite_pack/1darray/1darray.h"
#include "magic_number.h"
#include "rc.h"
#include <stdlib.h>

static int unpack_entry_dist(struct lip_file *file, enum entry_dist *ed)
{
  int rc = 0;
  if ((rc = expect_map_key(file, "entry_dist"))) return rc;
  if (!lip_read_int(file, ed)) return EFREAD;
  if (*ed <= ENTRY_DIST_NULL || *ed > ENTRY_DIST_OCCUPANCY) return EFDATA;
  return 0;
}

static int unpack_epsilon(struct lip_file *file, imm_float *epsilon)
{
  int rc = 0;
  if ((rc = expect_map_key(file, "epsilon"))) return rc;
  if (!lip_read_float(file, epsilon)) return EFREAD;

  return (*epsilon < 0 || *epsilon > 1) ? EFDATA : 0;
}

static int unpack_nuclt(struct lip_file *file, struct imm_nuclt *nuclt)
{
  int rc = 0;
  if ((rc = expect_map_key(file, "abc"))) return rc;
  if (imm_abc_unpack(&nuclt->super, file)) return EFREAD;
  return 0;
}

static int unpack_amino(struct lip_file *file, struct imm_amino *amino)
{
  int rc = 0;
  if ((rc = expect_map_key(file, "amino"))) return rc;
  if (imm_abc_unpack(&amino->super, file)) return EFREAD;
  return 0;
}

int db_reader_open(struct db_reader *db, FILE *fp)
{
  int rc = 0;

  db->nproteins = 0;
  db->protein_sizes = 0;
  lip_file_init(&db->file, fp);

  if ((rc = expect_map_size(&db->file, 2))) return rc;
  if ((rc = expect_map_key(&db->file, "header"))) return rc;
  if ((rc = expect_map_size(&db->file, 7))) return rc;
  if ((rc = db_reader_unpack_magic_number(db))) defer_return(rc);
  if ((rc = db_reader_unpack_float_size(db))) defer_return(rc);
  if ((rc = unpack_entry_dist(&db->file, &db->cfg.edist))) defer_return(rc);
  if ((rc = unpack_epsilon(&db->file, &db->cfg.eps))) defer_return(rc);
  if ((rc = unpack_nuclt(&db->file, &db->nuclt))) defer_return(rc);
  if ((rc = unpack_amino(&db->file, &db->amino))) defer_return(rc);
  imm_nuclt_code_init(&db->code, &db->nuclt);
  if ((rc = db_reader_unpack_prot_sizes(db))) defer_return(rc);

  return rc;

defer:
  db_reader_close(db);
  return rc;
}

void db_reader_close(struct db_reader *db)
{
  if (db->protein_sizes) free(db->protein_sizes);
}

int db_reader_unpack_magic_number(struct db_reader *db)
{
  int rc = 0;

  if ((rc = expect_map_key(&db->file, "magic_number"))) return rc;

  unsigned number = 0;
  if (!lip_read_int(&db->file, &number)) return EFREAD;

  return number != MAGIC_NUMBER ? EFDATA : 0;
}

int db_reader_unpack_float_size(struct db_reader *db)
{
  int rc = 0;
  if ((rc = expect_map_key(&db->file, "float_size"))) return rc;

  unsigned size = 0;
  if (!lip_read_int(&db->file, &size)) return EFREAD;

  return size != IMM_FLOAT_BYTES ? EFDATA : 0;
}

static int unpack_header_protein_sizes(struct db_reader *db)
{
  enum lip_1darray_type type = 0;

  if (!lip_read_1darray_size_type(&db->file, &db->nproteins, &type))
    return EFREAD;

  if (type != LIP_1DARRAY_UINT32) return EFDATA;

  db->protein_sizes = malloc(sizeof(*db->protein_sizes) * db->nproteins);
  if (!db->protein_sizes) return ENOMEM;

  if (!lip_read_1darray_u32_data(&db->file, db->nproteins, db->protein_sizes))
  {
    free(db->protein_sizes);
    db->protein_sizes = 0;
    return EFREAD;
  }

  return 0;
}

int db_reader_unpack_prot_sizes(struct db_reader *db)
{
  int rc = 0;
  if ((rc = expect_map_key(&db->file, "protein_sizes"))) return rc;
  return unpack_header_protein_sizes(db);
}
