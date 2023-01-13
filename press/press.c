#include "press.h"
#include "defer_return.h"
#include "fs.h"
#include "rc.h"
#include "sizeof_field.h"
#include "strlcpy.h"
#include <string.h>

static int count_profiles(struct db_press *);
static int prepare_writer(struct db_press *);
static int finish_writer(struct db_press *, bool succesfully);
static void prepare_reader(struct db_press *);
static int finish_reader(struct db_press *);
static int prof_write(struct db_press *);

int db_press_init(struct db_press *p, char const *hmm, char const *db)
{
  p->writer.fp = NULL;
  p->reader.fp = NULL;

  int rc = 0;

  if (!(p->reader.fp = fopen(hmm, "rb"))) defer_return(EOPENHMM);
  if (!(p->writer.fp = fopen(db, "wb"))) defer_return(EOPENDB);

  if ((rc = count_profiles(p))) defer_return(rc);
  if ((rc = prepare_writer(p))) defer_return(rc);

  prepare_reader(p);

  prot_prof_init(&p->prof, p->reader.h3.prof.meta.acc, &p->writer.db.amino,
                 &p->writer.db.code, p->writer.db.cfg);

  return rc;

defer:
  if (p->writer.fp) fclose(p->writer.fp);
  if (p->reader.fp) fclose(p->reader.fp);
  p->writer.fp = NULL;
  p->reader.fp = NULL;
  return rc;
}

long db_press_nsteps(struct db_press const *p) { return p->prof_count; }

static int count_profiles(struct db_press *p)
{
#define HMMER3 "HMMER3/f"
  unsigned count = 0;

  while (fgets(p->buffer, sizeof_field(struct db_press, buffer),
               p->reader.fp) != NULL)
  {
    if (!strncmp(p->buffer, HMMER3, strlen(HMMER3))) ++count;
  }

  if (!feof(p->reader.fp)) return EEOF;

  p->prof_count = count;
  rewind(p->reader.fp);
  return 0;
#undef HMMER3
}

int db_press_step(struct db_press *p)
{
  int rc = prot_h3reader_next(&p->reader.h3);
  return rc ? rc : prof_write(p);
}

int db_press_cleanup(struct db_press *p, bool succesfully)
{
  int rc_r = finish_reader(p);
  int rc_w = finish_writer(p, succesfully);
  return rc_r ? rc_r : (rc_w ? rc_w : 0);
}

static int prepare_writer(struct db_press *p)
{
  struct imm_amino const *a = &imm_amino_iupac;
  struct imm_nuclt const *n = &imm_dna_iupac.super;
  struct prot_cfg cfg = PROT_CFG_DEFAULT;

  return prot_db_writer_open(&p->writer.db, p->writer.fp, a, n, cfg);
}

static int finish_writer(struct db_press *p, bool succesfully)
{
  if (!p->writer.fp) return 0;
  if (!succesfully)
  {
    db_writer_close((struct db_writer *)&p->writer.db, false);
    fclose(p->writer.fp);
    return 0;
  }
  int rc = db_writer_close((struct db_writer *)&p->writer.db, true);
  if (rc)
  {
    fclose(p->writer.fp);
    return rc;
  }
  return fs_close(p->writer.fp);
}

static void prepare_reader(struct db_press *p)
{
  struct imm_amino const *amino = &p->writer.db.amino;
  struct imm_nuclt_code const *code = &p->writer.db.code;
  struct prot_cfg cfg = p->writer.db.cfg;

  prot_h3reader_init(&p->reader.h3, amino, code, cfg, p->reader.fp);
}

static int finish_reader(struct db_press *p) { return fs_close(p->reader.fp); }

static int prof_write(struct db_press *p)
{
  int rc = prot_prof_absorb(&p->prof, &p->reader.h3.model);
  if (rc) return rc;

  strlcpy(p->prof.super.acc, p->reader.h3.prof.meta.acc, PROF_ACC_SIZE);

  return prot_db_writer_pack_profile(&p->writer.db, &p->prof);
}
