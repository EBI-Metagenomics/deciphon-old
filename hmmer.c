#include "hmmer.h"
#include "deciphon/errno.h"
#include "h3c/h3c.h"
#include "hmmer_dialer.h"
#include "hmmer_result.h"
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
  fprintf(stderr, "(%p) seq: %s\n", x->stream, seq);
  if (rc)
  {
    fprintf(stderr, "(%p) h3c_stream_put error: %s\n", x->stream,
            h3c_strerror(rc));
    return rc;
  }
  return 0;
}

int hmmer_warmup(struct hmmer *x)
{
  int rc = 0;
  if ((rc = put(x, 0, "noname", "", WARMUP_DEADLINE)))
  {
    fprintf(stderr, "hmmer_warmup put error: %s", h3c_strerror(rc));
    return rc;
  }
  if ((rc = hmmer_pop(x)))
  {
    fprintf(stderr, "hmmer_warmup pop error: %s", h3c_strerror(rc));
    return rc;
  }
  return rc;
}

int hmmer_get(struct hmmer *x, int hmmidx, char const *name, char const *seq)
{
  for (int i = 0; i < NUM_TRIALS; ++i)
  {
    if (i > 0) fprintf(stderr, "(%p) trying again on hmmer_get\n", x->stream);

    int rc = 0;
    if ((rc = put(x, hmmidx, name, seq, REQUEST_DEADLINE)))
    {
      fprintf(stderr, "(%p) giving up on hmmer_put\n", x->stream);
      return rc;
    }

    if ((rc = hmmer_pop(x)) == H3C_ETIMEDOUT) continue;

    if (rc)
    {
      fprintf(stderr, "(%p) giving up on hmmer_pop\n", x->stream);
      return rc;
    }

    return 0;
  }

  fprintf(stderr, "(%p) reached maximum number of retries on hmmer_get\n",
          x->stream);
  return DCP_EMAXRETRY;
}

int hmmer_put(struct hmmer *x, int hmmidx, char const *name, char const *seq)
{
  return put(x, hmmidx, name, seq, REQUEST_DEADLINE) ? DCP_EH3CPUT : 0;
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
