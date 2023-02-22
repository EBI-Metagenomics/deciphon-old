#ifndef HMMER_H
#define HMMER_H

#include "hmmer_result.h"

struct hmmer
{
  struct h3c_stream *stream;
  struct hmmer_result result;
};

int hmmer_init(struct hmmer *);
void hmmer_cleanup(struct hmmer *);

int hmmer_put(struct hmmer *, int hmmidx, char const *name, char const *seq);
int hmmer_pop(struct hmmer *);

#endif
