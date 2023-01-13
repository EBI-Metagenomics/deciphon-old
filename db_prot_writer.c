#include "db_prot_writer.h"
#include "defer_return.h"
#include "prof_typeid.h"
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

int prot_db_writer_open(struct prot_db_writer *db, FILE *fp,
                        struct imm_amino const *amino,
                        struct imm_nuclt const *nuclt, struct prot_cfg cfg)
{
  int rc = 0;

  if ((rc = db_writer_open(&db->super, fp))) return rc;

  db->amino = *amino;
  db->nuclt = *nuclt;
  imm_nuclt_code_init(&db->code, &db->nuclt);
  db->cfg = cfg;

  imm_float const *epsilon = &db->cfg.eps;
  enum entry_dist const *edist = &db->cfg.edist;

  struct db_writer *w = &db->super;
  struct imm_nuclt const *n = &db->nuclt;
  struct imm_amino const *a = &db->amino;
  if ((rc = db_writer_pack_magic_number(w))) defer_return(rc);
  if ((rc = db_writer_pack_prof_typeid(w, PROF_PROT))) defer_return(rc);
  if ((rc = db_writer_pack_float_size(w))) defer_return(rc);
  if ((rc = db_writer_pack_header(w, pack_edist_cb, edist))) defer_return(rc);
  if ((rc = db_writer_pack_header(w, pack_eps_cb, epsilon))) defer_return(rc);
  if ((rc = db_writer_pack_header(w, pack_nuclt_cb, n))) defer_return(rc);
  if ((rc = db_writer_pack_header(w, pack_amino_cb, a))) defer_return(rc);

  return rc;

defer:
  db_writer_close(w, false);
  return rc;
}

static int pack_profile(struct lip_file *file, void const *prof)
{
  return prot_prof_pack(prof, file);
}

int prot_db_writer_pack_profile(struct prot_db_writer *db,
                                struct prot_prof const *profile)
{
  return db_writer_pack_prof(&db->super, pack_profile, profile);
}
