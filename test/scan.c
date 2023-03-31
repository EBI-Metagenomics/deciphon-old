#include "deciphon/scan.h"
#include "deciphon/seq.h"
#include "fs.h"
#include "hope.h"
#include "test_seqs.h"
#include <stdlib.h>

void test_scan(void);

int main(void)
{
  test_scan();
  return hope_status();
}

static bool next_seq(struct dcp_seq *, void *);
static long checksum(char const *filename);

void test_scan(void)
{
  struct dcp_scan *scan = dcp_scan_new(51371);

  dcp_scan_set_nthreads(scan, 1);
  dcp_scan_set_lrt_threshold(scan, 10.);
  dcp_scan_set_multi_hits(scan, true);
  dcp_scan_set_hmmer3_compat(scan, false);

  eq(dcp_scan_set_db_file(scan, ASSETS "/minifam.dcp"), 0);
  dcp_scan_set_seq_iter(scan, next_seq, NULL);

  eq(dcp_scan_run(scan, "prod"), 0);
  eq(checksum("prod/products.tsv"), 2817);

  dcp_scan_del(scan);
}

static bool next_seq(struct dcp_seq *x, void *arg)
{
  static int i = 0;
  (void)arg;
  if (i > 2) return false;
  dcp_seq_setup(x, test_seqs[i].id, test_seqs[i].name, test_seqs[i].data);
  i += 1;
  return true;
}

static long checksum(char const *filename)
{
  long chk = 0;
  if (fs_cksum(filename, &chk))
  {
    fprintf(stderr, "Failed to compute file checksum.\n");
    exit(1);
  }
  return chk;
}
