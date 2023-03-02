#include "seq_iter.h"
#include "deciphon/seq.h"
#include "seq.h"

static bool noop(struct dcp_seq *x, void *arg)
{
  (void)x;
  (void)arg;
  return false;
}

void seq_iter_init(struct seq_iter *x, dcp_seq_next_fn *callb, void *arg)
{
  dcp_seq_setup(&x->seq, 0, "", "");
  x->next_callb = callb ? callb : noop;
  x->arg = arg;
}

bool seq_iter_next(struct seq_iter *x)
{
  return (*x->next_callb)(&x->seq, x->arg);
}

struct dcp_seq const *seq_iter_get(struct seq_iter *x) { return &x->seq; }
