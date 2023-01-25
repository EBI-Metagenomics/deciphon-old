#include "deciphon/scan.h"
#include "array_size_field.h"
#include "deciphon/errno.h"
#include "defer_return.h"
#include "scan_db.h"
#include "seqlist.h"
#include "strlcpy.h"
#include <stdlib.h>

struct dcp_scan
{
  int nthreads;
  struct thread *threads;

  double lrt_threshold;
  bool multi_hits;
  bool hmmer3_compat;

  char sequences[FILENAME_MAX];

  struct scan_db db;
  struct seqlist seqlist;
};

struct dcp_scan *dcp_scan_new(void)
{
  struct dcp_scan *x = malloc(sizeof(*x));
  x->nthreads = 1;
  // TODO: x->threads
  x->lrt_threshold = 10.;
  x->multi_hits = true;
  x->hmmer3_compat = false;
  x->sequences[0] = 0;
  scan_db_init(&x->db);
  seqlist_init(&x->seqlist);
  return x;
}

void dcp_scan_del(struct dcp_scan const *x) { free((void *)x); }

void dcp_scan_set_nthreads(struct dcp_scan *x, int nthreads)
{
  x->nthreads = nthreads;
}

void dcp_scan_set_lrt_threshold(struct dcp_scan *x, double lrt)
{
  x->lrt_threshold = lrt;
}

void dcp_scan_set_multi_hits(struct dcp_scan *x, bool multihits)
{
  x->multi_hits = multihits;
}

void dcp_scan_set_hmmer3_compat(struct dcp_scan *x, bool h3compat)
{
  x->hmmer3_compat = h3compat;
}

int dcp_scan_set_db_file(struct dcp_scan *x, char const *filename)
{
  return scan_db_set_filename(&x->db, filename);
}

int dcp_scan_set_seq_file(struct dcp_scan *x, char const *filename)
{
  size_t n = array_size_field(struct dcp_scan, sequences);
  return strlcpy(x->sequences, filename, n) < n ? 0 : DCP_ELARGEPATH;
}

int dcp_scan_run(struct dcp_scan *x)
{
  int rc = 0;

  if ((rc = scan_db_open(&x->db, x->nthreads))) return rc;
  if ((rc = seqlist_open(&x->seqlist, x->sequences))) defer_return(rc);

defer:
  seqlist_close(&x->seqlist);
  scan_db_close(&x->db);
  return rc;
}
