#include "hmmer.h"
#include "deciphon/errno.h"
#include "h3c/h3c.h"
#include "hmmer_dialer.h"
#include "hmmer_result.h"
#include <stdio.h>
#include <stdlib.h>

#define DEADLINE 30000

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
  if (elapsed_start(&x->elapsed)) return DCP_EELAPSED;
  int rc = h3c_stream_put(x->stream, cmd, name, seq, h3c_deadline(DEADLINE));
  fprintf(stderr, "(%p) seq: %s\n", x->stream, seq);
  if (rc)
  {
    fprintf(stderr, "(%p) h3c_stream_put error: %s\n", x->stream,
            h3c_strerror(rc));
    return DCP_EH3CPUT;
  }
  return 0;
}

int hmmer_pop(struct hmmer *x)
{
  h3c_stream_wait(x->stream);
  int rc = h3c_stream_pop(x->stream, x->result.handle);
  if (elapsed_stop(&x->elapsed)) return DCP_EELAPSED;
  long mseconds = elapsed_milliseconds(&x->elapsed);
  if (rc)
  {
    fprintf(stderr, "(%p) h3c_stream_pop error [%ld msecs]: %s\n", x->stream,
            mseconds, h3c_strerror(rc));
    return DCP_EH3CPOP;
  }
  else
  {
    fprintf(stderr, "(%p) hmmer took %ld msecs\n", x->stream, mseconds);
  }
  return 0;
}
