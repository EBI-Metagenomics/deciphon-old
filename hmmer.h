#ifndef HMMER_H
#define HMMER_H

#include "elapsed/elapsed.h"
#include "hmmer_result.h"

struct hmmer
{
  struct h3c_stream *stream;
  struct hmmer_result result;
  struct elapsed elapsed;
};

int hmmer_init(struct hmmer *);
void hmmer_cleanup(struct hmmer *);
int hmmer_warmup(struct hmmer *);
int hmmer_get(struct hmmer *, int hmmidx, char const *name, char const *seq);

#endif
