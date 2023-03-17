#include "deciphon/press.h"
#include "hope.h"

int main(void)
{
  struct dcp_press *press = dcp_press_new();

  char const *hmm = "/Users/horta/data/Pfam-shortest.hmm";
  char const *db = TMPDIR "/Pfam-shortest.dcp";
  int rc = dcp_press_open(press, hmm, db);
  eq(rc, 0);

  eq(dcp_press_nproteins(press), 1);
  while (!dcp_press_end(press))
  {
    if ((rc = dcp_press_next(press))) break;
  }
  eq(rc, 0);

  eq(dcp_press_close(press), 0);
  dcp_press_del(press);

  return hope_status();
}
