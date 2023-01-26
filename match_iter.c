#include "match_iter.h"
#include "protein.h"

void match_iter_init(struct match_iter *x, struct protein const *protein,
                     struct imm_seq const *seq, struct imm_path const *path)
{
  x->seq = seq;
  x->path = path;
  match_init(&x->match, protein);
  x->idx = 0;
  x->offset = 0;
}

int match_iter_next(struct match_iter *x)
{
  if (match_iter_end(x)) return 0;

  struct imm_path const *path = x->path;

  struct imm_step const *step = imm_path_step(path, x->idx);
  struct imm_seq seq = imm_subseq(x->seq, x->offset, step->seqlen);

  x->idx += 1;
  x->offset += step->seqlen;

  return match_setup(&x->match, *step, seq);
}

bool match_iter_end(struct match_iter const *x)
{
  return x->idx >= imm_path_nsteps(x->path);
}

struct match const *match_iter_get(struct match_iter *x) { return &x->match; }
