#include "deciphon/scan.h"
#include "db_reader.h"
#include "deciphon/errno.h"
#include "defer_return.h"
#include "protein_reader.h"
#include "seqlist.h"
#include <stdlib.h>

struct dcp_scan
{
  int nthreads;
  struct thread *threads;

  double lrt_threshold;
  bool multihits;
  bool hmmer3_compat;

  char const *database;
  char const *sequences;

  struct db_reader db_reader;
  struct protein_reader proto_reader;
  struct seqlist seqlist;
};

struct dcp_scan *dcp_scan_new(void)
{
  struct dcp_scan *x = malloc(sizeof(*x));
  x->nthreads = 1;
  // TODO: x->threads
  x->lrt_threshold = 10.;
  x->multihits = true;
  x->hmmer3_compat = false;
  x->database = NULL;
  x->sequences = NULL;
  seqlist_init(&x->seqlist);
  return x;
}

int dcp_scan_run(struct dcp_scan *x)
{
  int rc = 0;

  FILE *file = fopen(x->database, "rb");
  if (!file) return DCP_EOPENDB;

  if ((rc = db_reader_open(&x->db_reader, file))) defer_return(rc);

  if ((rc = protein_reader_open(&x->proto_reader, &x->db_reader, x->nthreads)))
    defer_return(rc);

  if ((rc = seqlist_open(&x->seqlist, x->sequences))) defer_return(rc);

defer:
  seqlist_close(&x->seqlist);
  protein_reader_close(&x->proto_reader);
  db_reader_close(&x->db_reader);
  return rc;
}
