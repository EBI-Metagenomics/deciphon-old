#ifndef SEQ_ITER_H
#define SEQ_ITER_H

#include "deciphon/seq.h"
#include "seq.h"

struct seq_iter
{
  struct dcp_seq seq;
  dcp_seq_next_fn *next_callb;
  void *arg;
};

void seq_iter_init(struct seq_iter *, dcp_seq_next_fn *, void *);
bool seq_iter_next(struct seq_iter *);
struct dcp_seq const *seq_iter_get(struct seq_iter *);

#endif
