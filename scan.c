#include "deciphon/scan.h"
#include "array_size_field.h"
#include "deciphon/errno.h"
#include "deciphon/limits.h"
#include "defer_return.h"
#include "scan_db.h"
#include "scan_thread.h"
#include "seq_list.h"
#include "strlcpy.h"
#include <stdlib.h>

struct dcp_scan
{
  int nthreads;
  struct scan_thread threads[DCP_NTHREADS_MAX];

  double lrt_threshold;
  bool multi_hits;
  bool hmmer3_compat;

  struct scan_db db;
  struct seq_list seqlist;
};

struct dcp_scan *dcp_scan_new(void)
{
  struct dcp_scan *x = malloc(sizeof(*x));
  x->nthreads = 1;
  x->lrt_threshold = 10.;
  x->multi_hits = true;
  x->hmmer3_compat = false;
  scan_db_init(&x->db);
  seq_list_init(&x->seqlist);
  return x;
}

void dcp_scan_del(struct dcp_scan const *x) { free((void *)x); }

void dcp_scan_set_nthreads(struct dcp_scan *x, int nthreads)
{
  assert(nthreads <= DCP_NTHREADS_MAX);
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
  return seq_list_set_filename(&x->seqlist, filename);
}

int dcp_scan_run(struct dcp_scan *x)
{
  int rc = 0;

  if ((rc = scan_db_open(&x->db, x->nthreads))) defer_return(rc);
  if ((rc = seq_list_open(&x->seqlist))) defer_return(rc);

  int nparts = protein_reader_npartitions(scan_db_reader(&x->db));
  for (int i = 0; i < nparts; ++i)
  {
    struct scan_thread *t = x->threads + i;
    scan_thread_init(t, scan_db_reader(&x->db), i);
  }

  while (!seq_list_end(&x->seqlist))
  {
    if ((rc = seq_list_next(&x->seqlist))) break;

    struct imm_abc const *abc = scan_db_abc(&x->db);
    struct imm_str str = imm_str(seq_list_seq_data(&x->seqlist));
    struct imm_seq seq = imm_seq(str, abc);

    for (int i = 0; i < nparts; ++i)
    {
      struct scan_thread *t = x->threads + i;
      scan_thread_run(t, &seq);
    }
  }

#if 0
  long ntasks = 0;
  long ntasks_total = 0;
  int prof_start = 0;
  for (int i = 0; i < nparts; ++i)
  {
    struct scan_thread *t = x->threads + i;

    thread_init(t, prodman_file(i), i, prof_start, x->db.protein, scan_cfg);

    enum imm_abc_typeid abc_typeid = abc->vtable.typeid;
    ntasks = protein_reader_partition_size(&x->db.protein, i) *
             seq_list_size(&x->seqlist);
    assert(ntasks > 0);
    thread_setup_job(t, abc_typeid, prof_typeid, seqlist_scan_id(), ntasks);

    ntasks_total += ntasks;
    prof_start += protein_reader_partition_size(&x->db.protein, i);
  }
#endif

defer:
  seq_list_close(&x->seqlist);
  scan_db_close(&x->db);
  return rc;
}
