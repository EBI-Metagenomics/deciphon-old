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

long dcp_press_nproteins(struct dcp_press const *press) { return press->count; }

static int count_proteins(struct dcp_press *press)
{
#define HMMER3 "HMMER3/f"

  unsigned count = 0;
  int bufsize = sizeof_field(struct dcp_press, buffer);
  while (fgets(press->buffer, bufsize, press->reader.fp) != NULL)
  {
    if (!strncmp(press->buffer, HMMER3, strlen(HMMER3))) ++count;
  }

  if (!feof(press->reader.fp)) return EEOF;

  press->count = count;
  rewind(press->reader.fp);
  return 0;

#undef HMMER3
}

int dcp_press_next(struct dcp_press *press)
{
  int rc = h3reader_next(&press->reader.h3);
  if (rc) return rc;

  if (h3reader_end(&press->reader.h3)) return 0;

  return protein_write(press);
}

bool dcp_press_end(struct dcp_press const *press)
{
  return h3reader_end(&press->reader.h3);
}

int dcp_press_close(struct dcp_press *press, int succesfully)
{
  int rc_r = press->reader.fp ? fs_close(press->reader.fp) : 0;
  int rc_w = finish_writer(press, succesfully);
  press->writer.fp = NULL;
  press->reader.fp = NULL;
  return rc_r ? rc_r : (rc_w ? rc_w : 0);
}

void dcp_press_del(struct dcp_press const *x) { free((void *)x); }

static int prepare_writer(struct dcp_press *press)
{
  struct imm_amino const *a = &imm_amino_iupac;
  struct imm_nuclt const *n = &imm_dna_iupac.super;
  struct cfg cfg = CFG_DEFAULT;

  return db_writer_open(&press->writer.db, press->writer.fp, a, n, cfg);
}

static int finish_writer(struct dcp_press *press, bool succesfully)
{
  if (!press->writer.fp) return 0;
  if (!succesfully)
  {
    db_writer_close(&press->writer.db, false);
    fclose(press->writer.fp);
    return 0;
  }
  int rc = db_writer_close(&press->writer.db, true);
  if (rc)
  {
    fclose(press->writer.fp);
    return rc;
  }
  return fs_close(press->writer.fp);
}

static void prepare_reader(struct dcp_press *press)
{
  struct imm_amino const *amino = &press->writer.db.amino;
  struct imm_nuclt_code const *code = &press->writer.db.code;
  struct cfg cfg = press->writer.db.cfg;

  h3reader_init(&press->reader.h3, amino, code, cfg, press->reader.fp);
}

static int protein_write(struct dcp_press *x)
{
  int rc = protein_absorb(&x->protein, &x->reader.h3.model);
  if (rc) return rc;

  strlcpy(x->protein.accession, x->reader.h3.protein.meta.acc, ACCESSION_SIZE);

  return db_writer_pack(&x->writer.db, &x->protein);
}
