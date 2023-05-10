#include "db_reader.h"
#include "deciphon/errno.h"
#include "defer_return.h"
#include "expect.h"
#include "imm/imm.h"
#include "lip/1darray/1darray.h"
#include "magic_number.h"
#include <stdlib.h>

static int unpack_entry_dist(struct lip_file *file, enum entry_dist *ed)
{
  int rc = 0;
  if ((rc = expect_map_key(file, "entry_dist"))) return rc;
  if (!lip_read_int(file, ed)) return DCP_EFREAD;
  if (*ed <= ENTRY_DIST_NULL || *ed > ENTRY_DIST_OCCUPANCY) return DCP_EFDATA;
  return 0;
}

static int unpack_epsilon(struct lip_file *file, float *epsilon)
{
  int rc = 0;
  if ((rc = expect_map_key(file, "epsilon"))) return rc;
  if (!lip_read_float(file, epsilon)) return DCP_EFREAD;

  return (*epsilon < 0 || *epsilon > 1) ? DCP_EFDATA : 0;
}

static int unpack_nuclt(struct lip_file *file, struct imm_nuclt *nuclt)
{
  int rc = 0;
  if ((rc = expect_map_key(file, "abc"))) return rc;
  if (imm_abc_unpack(&nuclt->super, file)) return DCP_EFREAD;
  return 0;
}

static int unpack_amino(struct lip_file *file, struct imm_amino *amino)
{
  int rc = 0;
  if ((rc = expect_map_key(file, "amino"))) return rc;
  if (imm_abc_unpack(&amino->super, file)) return DCP_EFREAD;
  return 0;
}

void db_reader_init(struct db_reader *x)
{
  x->nproteins = 0;
  x->protein_sizes = NULL;
}

int db_reader_open(struct db_reader *x, FILE *fp)
{
  int rc = 0;

  x->nproteins = 0;
  x->protein_sizes = NULL;
  lip_file_init(&x->file, fp);

  if ((rc = expect_map_size(&x->file, 2))) return rc;
  if ((rc = expect_map_key(&x->file, "header"))) return rc;
  if ((rc = expect_map_size(&x->file, 7))) return rc;
  if ((rc = db_reader_unpack_magic_number(x))) defer_return(rc);
  if ((rc = db_reader_unpack_float_size(x))) defer_return(rc);
  if ((rc = unpack_entry_dist(&x->file, &x->entry_dist))) defer_return(rc);
  if ((rc = unpack_epsilon(&x->file, &x->epsilon))) defer_return(rc);
  if ((rc = unpack_nuclt(&x->file, &x->nuclt))) defer_return(rc);
  if ((rc = unpack_amino(&x->file, &x->amino))) defer_return(rc);
  imm_nuclt_code_init(&x->code, &x->nuclt);
  if ((rc = db_reader_unpack_prot_sizes(x))) defer_return(rc);

  return rc;

defer:
  db_reader_close(x);
  return rc;
}

void db_reader_close(struct db_reader *x)
{
  if (x->protein_sizes) free(x->protein_sizes);
  x->protein_sizes = NULL;
}

int db_reader_unpack_magic_number(struct db_reader *x)
{
  int rc = 0;

  if ((rc = expect_map_key(&x->file, "magic_number"))) return rc;

  unsigned number = 0;
  if (!lip_read_int(&x->file, &number)) return DCP_EFREAD;

  return number != MAGIC_NUMBER ? DCP_EFDATA : 0;
}

int db_reader_unpack_float_size(struct db_reader *x)
{
  int rc = 0;
  if ((rc = expect_map_key(&x->file, "float_size"))) return rc;

  unsigned size = 0;
  if (!lip_read_int(&x->file, &size)) return DCP_EFREAD;

  // return size != IMM_FLOAT_BYTES ? DCP_EFDATA : 0;
  return size != 4 ? DCP_EFDATA : 0;
}

static int unpack_header_protein_sizes(struct db_reader *x)
{
  enum lip_1darray_type type = 0;

  unsigned n = 0;
  if (!lip_read_1darray_size_type(&x->file, &n, &type)) return DCP_EFREAD;
  if (n > INT_MAX) return DCP_EFDATA;
  x->nproteins = (int)n;

  if (type != LIP_1DARRAY_UINT32) return DCP_EFDATA;

  x->protein_sizes = malloc(sizeof(*x->protein_sizes) * x->nproteins);
  if (!x->protein_sizes) return DCP_ENOMEM;

  if (!lip_read_1darray_u32_data(&x->file, x->nproteins, x->protein_sizes))
  {
    free(x->protein_sizes);
    x->protein_sizes = 0;
    return DCP_EFREAD;
  }

  return 0;
}

int db_reader_unpack_prot_sizes(struct db_reader *x)
{
  int rc = 0;
  if ((rc = expect_map_key(&x->file, "protein_sizes"))) return rc;
  return unpack_header_protein_sizes(x);
}
