#include "deciphon/scan.h"
#include "fs.h"
#include "hope.h"
#include <stdlib.h>

void test_scan(void);

int main(void)
{
  test_scan();
  return hope_status();
}

static long checksum(char const *filename);

void test_scan(void)
{
  struct dcp_scan *scan = dcp_scan_new(51371);

  dcp_scan_set_nthreads(scan, 1);
  dcp_scan_set_lrt_threshold(scan, 10.);
  dcp_scan_set_multi_hits(scan, true);
  dcp_scan_set_hmmer3_compat(scan, false);

  eq(dcp_scan_set_db_file(scan, ASSETS "/minifam.dcp"), 0);
  eq(dcp_scan_set_seq_file(scan, ASSETS "/consensus.json"), 0);

  eq(dcp_scan_run(scan, "prod"), 0);
  // eq(checksum(TMPDIR "/prod.tsv"), 57604);

  dcp_scan_del(scan);
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
