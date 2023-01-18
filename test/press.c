#include "deciphon/press.h"
#include "hope.h"

int main(void)
{
  struct dcp_press *press = dcp_press_new();

  // int rc = dcp_press_open(press, hmm, db);

  dcp_press_del(press);
  return hope_status();
}
