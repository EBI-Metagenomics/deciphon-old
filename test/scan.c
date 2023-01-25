#include "deciphon/scan.h"
#include "hope.h"

void test_scan(void);

int main(void)
{
  test_scan();
  return hope_status();
}

#define DB "/Users/horta/code/deciphon/build/minifam.dcp"
#define SEQ "/Users/horta/code/deciphon/build/consensus.json"

void test_scan(void)
{
  struct dcp_scan *scan = dcp_scan_new();

  dcp_scan_set_nthreads(scan, 1);
  dcp_scan_set_lrt_threshold(scan, 10.);
  dcp_scan_set_multi_hits(scan, true);
  dcp_scan_set_hmmer3_compat(scan, false);

  eq(dcp_scan_set_db_file(scan, DB), 0);
  eq(dcp_scan_set_seq_file(scan, SEQ), 0);

  eq(dcp_scan_run(scan), 0);

  dcp_scan_del(scan);
}
