#include "deciphon/scan.h"
#include "array_size_field.h"
#include "deciphon/errno.h"
#include "deciphon/limits.h"
#include "defer_return.h"
#include "prod_file.h"
#include "prod_thrd.h"
#include "scan_db.h"
#include "scan_thrd.h"
#include "seq_list.h"
#include "strlcpy.h"
#include <stdlib.h>

struct dcp_scan
{
  int nthreads;
  struct scan_thrd threads[DCP_NTHREADS_MAX];
  struct prod_file prod_file;

  double lrt_threshold;
  bool multi_hits;
  bool hmmer3_compat;

  struct scan_db db;
  struct seq_list seq_list;
};

struct dcp_scan *dcp_scan_new(void)
{
  struct dcp_scan *x = malloc(sizeof(*x));
  x->nthreads = 1;
  x->lrt_threshold = 10.;
  x->multi_hits = true;
  x->hmmer3_compat = false;
  scan_db_init(&x->db);
  seq_list_init(&x->seq_list);
  prod_file_init(&x->prod_file);
  return x;
}

void dcp_scan_del(struct dcp_scan const *x) { free((void *)x); }

int dcp_scan_set_nthreads(struct dcp_scan *x, int nthreads)
{
  if (nthreads > DCP_NTHREADS_MAX) return DCP_EMANYTHREADS;
  x->nthreads = nthreads;
  return 0;
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
  return seq_list_set_filename(&x->seq_list, filename);
}

static int save_output(struct prod_file *prod_file, char const *outfile);

int dcp_scan_run(struct dcp_scan *x, char const *outfile)
{
  int rc = 0;

  if ((rc = scan_db_open(&x->db, x->nthreads))) defer_return(rc);
  if ((rc = seq_list_open(&x->seq_list))) defer_return(rc);

  for (int i = 0; i < x->nthreads; ++i)
  {
    long scan_id = seq_list_scan_id(&x->seq_list);
    struct scan_thrd *t = x->threads + i;
    scan_thrd_init(t, scan_db_reader(&x->db), i, scan_id);
    scan_thrd_set_lrt_threshold(t, x->lrt_threshold);
    scan_thrd_set_multi_hits(t, x->multi_hits);
    scan_thrd_set_hmmer3_compat(t, x->hmmer3_compat);
  }

  if ((rc = prod_file_setup(&x->prod_file, x->nthreads))) defer_return(rc);

  while (!(rc = seq_list_next(&x->seq_list)))
  {
    if (seq_list_end(&x->seq_list)) break;

    struct imm_abc const *abc = scan_db_abc(&x->db);
    struct imm_str str = imm_str(seq_list_seq_data(&x->seq_list));
    struct imm_seq seq = imm_seq(str, abc);

    for (int i = 0; i < x->nthreads; ++i)
    {
      struct scan_thrd *t = x->threads + i;
      scan_thrd_set_seq_id(t, seq_list_seq_id(&x->seq_list));
      scan_thrd_run(t, &seq, prod_file_thread(&x->prod_file, i));
    }
  }
  if (rc) defer_return(rc);

  rc = save_output(&x->prod_file, outfile);

defer:
  prod_file_cleanup(&x->prod_file);
  seq_list_close(&x->seq_list);
  scan_db_close(&x->db);
  return rc;
}

static int save_output(struct prod_file *prod_file, char const *outfile)
{
  FILE *fp = fopen(outfile, "wb");
  if (!fp) return DCP_EFOPEN;

  int rc = 0;
  if ((rc = prod_file_finishup(prod_file, fp)))
  {
    fclose(fp);
    return rc;
  }

  return fclose(fp) ? DCP_EFCLOSE : 0;
}
