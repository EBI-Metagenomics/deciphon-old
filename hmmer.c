#include "hmmer.h"
#include "deciphon/errno.h"
#include "h3c/h3c.h"
#include "hmmer_dialer.h"
#include "hmmer_result.h"
#include "ouch.h"
#include <stdio.h>
#include <stdlib.h>

#define NUM_TRIALS 5
#define REQUEST_DEADLINE 30000
#define WARMUP_DEADLINE 5000

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

static int put(struct hmmer *x, int hmmidx, char const *name, char const *seq,
               int deadline)
{
  char cmd[128] = {0};
  sprintf(cmd, "--hmmdb 1 --hmmdb_range %d..%d --acc --cut_ga", hmmidx, hmmidx);
  if (elapsed_start(&x->elapsed)) return DCP_EELAPSED;
  int rc = h3c_stream_put(x->stream, cmd, name, seq, h3c_deadline(deadline));
  ouch("(%p) seq: %s", x->stream, seq);
  if (rc)
  {
    ouch("(%p) h3c_stream_put error: %s", x->stream, h3c_strerror(rc));
    return rc;
  }
  return 0;
}

static int pop(struct hmmer *x)
{
  h3c_stream_wait(x->stream);
  int rc = h3c_stream_pop(x->stream, x->result.handle);
  if (elapsed_stop(&x->elapsed)) return DCP_EELAPSED;
  long mseconds = elapsed_milliseconds(&x->elapsed);
  if (rc)
  {
    ouch("(%p) h3c_stream_pop error [%ld msecs]: %s", x->stream, mseconds,
         h3c_strerror(rc));
    return rc;
  }
  else
  {
    ouch("(%p) hmmer took %ld msecs", x->stream, mseconds);
  }
  return 0;
}

int hmmer_warmup(struct hmmer *x)
{
  int rc = 0;
  if ((rc = put(x, 0, "noname", "", WARMUP_DEADLINE)))
  {
    ouch("hmmer_warmup put error: %s", h3c_strerror(rc));
    return rc;
  }
  if ((rc = pop(x)))
  {
    ouch("hmmer_warmup pop error: %s", h3c_strerror(rc));
    return rc;
  }
  return rc;
}

int hmmer_get(struct hmmer *x, int hmmidx, char const *name, char const *seq)
{
  for (int i = 0; i < NUM_TRIALS; ++i)
  {
    if (i > 0) ouch("(%p) trying again on hmmer_get", x->stream);

    int rc = 0;
    if ((rc = put(x, hmmidx, name, seq, REQUEST_DEADLINE)))
    {
      ouch("(%p) giving up on put", x->stream);
      return DCP_EH3CPUT;
    }

    if ((rc = pop(x)) == H3C_ETIMEDOUT) continue;

    if (rc)
    {
      ouch("(%p) giving up on pop", x->stream);
      return DCP_EH3CPOP;
    }

    return 0;
  }

  ouch("(%p) reached maximum number of retries on hmmer_get", x->stream);
  return DCP_EMAXRETRY;
}
