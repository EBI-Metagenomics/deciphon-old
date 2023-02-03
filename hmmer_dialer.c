#include "hmmer_dialer.h"
#include "deciphon/errno.h"
#include "h3c/h3c.h"
#include "hmmer.h"
#include <stdlib.h>

int hmmer_dialer_init(struct hmmer_dialer *x, int port)
{
  return (x->dialer = h3c_dialer_new("127.0.0.1", port)) ? 0 : DCP_ENOMEM;
}

void hmmer_dialer_cleanup(struct hmmer_dialer *x)
{
  if (x->dialer) h3c_dialer_del(x->dialer);
  x->dialer = NULL;
}

int hmmer_dialer_dial(struct hmmer_dialer *x, struct hmmer *y)
{
  int rc = 0;
  if ((rc = h3c_dialer_dial(x->dialer, h3c_deadline(1000)))) return rc;
  y->stream = h3c_dialer_stream(x->dialer);
  return rc;
}
