#include "deciphon/press.h"
#include "db_writer.h"
#include "defer_return.h"
#include "fs.h"
#include "h3reader.h"
#include "rc.h"
#include "sizeof_field.h"
#include "strlcpy.h"
#include <stdlib.h>
#include <string.h>

struct dcp_press
{
  struct
  {
    FILE *fp;
    struct db_writer db;
  } writer;

  struct
  {
    FILE *fp;
    struct h3reader h3;
  } reader;

  unsigned count;
  struct protein protein;

  char buffer[4 * 1024];
};

static int count_proteins(struct dcp_press *);
static int prepare_writer(struct dcp_press *);
static int finish_writer(struct dcp_press *, bool succesfully);
static void prepare_reader(struct dcp_press *);
static int protein_write(struct dcp_press *);

struct dcp_press *dcp_press_new(void)
{
  struct dcp_press *p = malloc(sizeof(struct dcp_press));
  if (!p) return NULL;

  p->writer.fp = NULL;
  p->reader.fp = NULL;
  return p;
}

int dcp_press_open(struct dcp_press *press, char const *hmm, char const *db)
{
  press->writer.fp = NULL;
  press->reader.fp = NULL;

  int rc = 0;

  if (!(press->reader.fp = fopen(hmm, "rb"))) defer_return(EOPENHMM);
  if (!(press->writer.fp = fopen(db, "wb"))) defer_return(EOPENDB);

  if ((rc = count_proteins(press))) defer_return(rc);
  if ((rc = prepare_writer(press))) defer_return(rc);

  prepare_reader(press);

  protein_init(&press->protein, press->reader.h3.protein.meta.acc,
               &press->writer.db.amino, &press->writer.db.code,
               press->writer.db.cfg);

  return rc;

defer:
  if (press->writer.fp) fclose(press->writer.fp);
  if (press->reader.fp) fclose(press->reader.fp);
  press->writer.fp = NULL;
  press->reader.fp = NULL;
  return rc;
}

long dcp_press_nsteps(struct dcp_press const *press) { return press->count; }

static int count_proteins(struct dcp_press *p)
{
#define HMMER3 "HMMER3/f"

  unsigned count = 0;
  int bufsize = sizeof_field(struct dcp_press, buffer);
  while (fgets(p->buffer, bufsize, p->reader.fp) != NULL)
  {
    if (!strncmp(p->buffer, HMMER3, strlen(HMMER3))) ++count;
  }

  if (!feof(p->reader.fp)) return EEOF;

  p->count = count;
  rewind(p->reader.fp);
  return 0;

#undef HMMER3
}

int dcp_press_step(struct dcp_press *p)
{
  int rc = h3reader_next(&p->reader.h3);
  return rc ? rc : protein_write(p);
}

int dcp_press_close(struct dcp_press *p, int succesfully)
{
  int rc_r = p->reader.fp ? fs_close(p->reader.fp) : 0;
  int rc_w = finish_writer(p, succesfully);
  p->writer.fp = NULL;
  p->reader.fp = NULL;
  return rc_r ? rc_r : (rc_w ? rc_w : 0);
}

void dcp_press_del(struct dcp_press const *p) { free((void *)p); }

static int prepare_writer(struct dcp_press *p)
{
  struct imm_amino const *a = &imm_amino_iupac;
  struct imm_nuclt const *n = &imm_dna_iupac.super;
  struct cfg cfg = CFG_DEFAULT;

  return db_writer_open(&p->writer.db, p->writer.fp, a, n, cfg);
}

static int finish_writer(struct dcp_press *p, bool succesfully)
{
  if (!p->writer.fp) return 0;
  if (!succesfully)
  {
    db_writer_close(&p->writer.db, false);
    fclose(p->writer.fp);
    return 0;
  }
  int rc = db_writer_close(&p->writer.db, true);
  if (rc)
  {
    fclose(p->writer.fp);
    return rc;
  }
  return fs_close(p->writer.fp);
}

static void prepare_reader(struct dcp_press *p)
{
  struct imm_amino const *amino = &p->writer.db.amino;
  struct imm_nuclt_code const *code = &p->writer.db.code;
  struct cfg cfg = p->writer.db.cfg;

  h3reader_init(&p->reader.h3, amino, code, cfg, p->reader.fp);
}

static int protein_write(struct dcp_press *p)
{
  int rc = protein_absorb(&p->protein, &p->reader.h3.model);
  if (rc) return rc;

  strlcpy(p->protein.accession, p->reader.h3.protein.meta.acc, ACCESSION_SIZE);

  return db_writer_pack(&p->writer.db, &p->protein);
}
