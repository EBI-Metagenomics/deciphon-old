#include "deciphon/seq_iter.h"
#include "deciphon/seq.h"
#include "seq.h"
#include <stdlib.h>

struct dcp_seq_iter
{
  struct dcp_seq seq;
  dcp_seq_iter_next_callb *next_callb;
  void *arg;
};

struct dcp_seq_iter *dcp_seq_iter_new(dcp_seq_iter_next_callb *callb, void *arg)
{
  struct dcp_seq_iter *x = malloc(sizeof(*x));
  if (!x) return x;
  dcp_seq_setup(&x->seq, 0, "", "");
  x->next_callb = callb;
  x->arg = arg;
  return x;
}

bool dcp_seq_iter_next(struct dcp_seq_iter *x)
{
  return (*x->next_callb)(&x->seq, x->arg);
}

void dcp_seq_iter_del(struct dcp_seq_iter const *x)
{
  if (x) free((void *)x);
}
