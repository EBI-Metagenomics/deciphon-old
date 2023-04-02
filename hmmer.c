#include "hmmer.h"
#include "deciphon/errno.h"
#include "h3c/h3c.h"
#include "hmmer_dialer.h"
#include "hmmer_result.h"
#include <stdlib.h>

int hmmer_init(struct hmmer *x)
{
  x->stream = NULL;
  return hmmer_result_init(&x->result);
}

void hmmer_cleanup(struct hmmer *x)
{
  if (x)
  {
    if (x->stream) h3c_stream_del(x->stream);
    x->stream = NULL;
    hmmer_result_cleanup(&x->result);
  }
}

int hmmer_put(struct hmmer *x, int hmmidx, char const *name, char const *seq)
{
  char cmd[128] = {0};
  sprintf(cmd, "--hmmdb 1 --hmmdb_range %d..%d --acc --cut_ga", hmmidx, hmmidx);
  return h3c_stream_put(x->stream, cmd, name, seq, h3c_deadline(15000))
             ? DCP_EH3CPUT
             : 0;
}

int hmmer_pop(struct hmmer *x)
{
  h3c_stream_wait(x->stream);
  return h3c_stream_pop(x->stream, x->result.handle) ? DCP_EH3CPOP : 0;
}
