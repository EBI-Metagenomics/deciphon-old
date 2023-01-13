#include "db_prot_reader.h"
#include "db_reader.h"
#include "defer_return.h"
#include "expect.h"
#include "imm/imm.h"
#include "rc.h"

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

int prot_db_reader_open(struct prot_db_reader *db, FILE *fp)
{
  int rc = db_reader_open(&db->super, fp);
  if (rc) return rc;

  struct db_reader *base = &db->super;
  if ((rc = expect_map_size(&base->file, 2))) return rc;
  if ((rc = expect_map_key(&base->file, "header"))) return rc;
  if ((rc = expect_map_size(&base->file, 8))) return rc;
  if ((rc = db_reader_unpack_magic_number(base))) defer_return(rc);
  if ((rc = db_reader_unpack_prof_typeid(base, PROF_PROT))) defer_return(rc);
  if ((rc = db_reader_unpack_float_size(base))) defer_return(rc);
  if ((rc = unpack_entry_dist(&base->file, &db->cfg.edist))) defer_return(rc);
  if ((rc = unpack_epsilon(&base->file, &db->cfg.eps))) defer_return(rc);
  if ((rc = unpack_nuclt(&base->file, &db->nuclt))) defer_return(rc);
  if ((rc = unpack_amino(&base->file, &db->amino))) defer_return(rc);
  imm_nuclt_code_init(&db->code, &db->nuclt);
  if ((rc = db_reader_unpack_prof_sizes(base))) defer_return(rc);

  return rc;

defer:
  db_reader_close(&db->super);
  return rc;
}
