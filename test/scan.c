#include "deciphon/scan.h"
#include "deciphon/seq.h"
#include "fs.h"
#include "hope.h"
#include "test_seqs.h"
#include <stdlib.h>

#include <sys/stat.h>

static void test_scan1(void);
static void test_scan2(void);
static void test_scan3(void);

int main(void)
{
  test_scan1();
  test_scan2();
  test_scan3();
  return hope_status();
}

static void seq_init(void);
static bool seq_next(struct dcp_seq *, void *);

static long fs_size(char const *filepath)
{
  struct stat st = {0};
  if (stat(filepath, &st)) return -1;
  return (long)st.st_size;
}

static void test_scan1(void)
{
  fprintf(stderr, "test_scan1\n");
  struct dcp_scan *scan = dcp_scan_new(51371);

  dcp_scan_set_nthreads(scan, 1);
  dcp_scan_set_lrt_threshold(scan, 10.);
  dcp_scan_set_multi_hits(scan, true);
  dcp_scan_set_hmmer3_compat(scan, false);

  eq(dcp_scan_set_db_file(scan, ASSETS "/minifam.dcp"), 0);
  seq_init();
  dcp_scan_set_seq_iter(scan, seq_next, NULL);

  eq(dcp_scan_run(scan, "prod1"), 0);
  eq(fs_size("prod1/products.tsv"), 8646);

  dcp_scan_del(scan);
}

static void test_scan2(void)
{
  fprintf(stderr, "test_scan2\n");
  struct dcp_scan *scan = dcp_scan_new(51371);

  dcp_scan_set_nthreads(scan, 2);
  dcp_scan_set_lrt_threshold(scan, 10.);
  dcp_scan_set_multi_hits(scan, true);
  dcp_scan_set_hmmer3_compat(scan, false);

  eq(dcp_scan_set_db_file(scan, ASSETS "/minifam.dcp"), 0);
  seq_init();
  dcp_scan_set_seq_iter(scan, seq_next, NULL);

  eq(dcp_scan_run(scan, "prod2"), 0);
  eq(fs_size("prod2/products.tsv"), 8646);

  dcp_scan_del(scan);
}

static void test_scan3(void)
{
  fprintf(stderr, "test_scan3\n");
  struct dcp_scan *scan = dcp_scan_new(51371);

  dcp_scan_set_nthreads(scan, 2);
  dcp_scan_set_lrt_threshold(scan, 2.);
  dcp_scan_set_multi_hits(scan, true);
  dcp_scan_set_hmmer3_compat(scan, false);
  dcp_scan_set_heuristic(scan, true);

  eq(dcp_scan_set_db_file(scan, ASSETS "/minifam.dcp"), 0);
  seq_init();
  dcp_scan_set_seq_iter(scan, seq_next, NULL);

  eq(dcp_scan_run(scan, "prod3"), 0);
  eq(fs_size("prod3/products.tsv"), 8646);

  dcp_scan_del(scan);
}

static int seq_idx = 0;

static void seq_init(void) { seq_idx = 0; }

static bool seq_next(struct dcp_seq *x, void *arg)
{
  (void)arg;
  if (seq_idx > 2) return false;
  int i = seq_idx;
  dcp_seq_setup(x, test_seqs[i].id, test_seqs[i].name, test_seqs[i].data);
  seq_idx = seq_idx + 1;
  return true;
}
