#ifndef MATCH_ITER_H
#define MATCH_ITER_H

#include "match.h"

struct imm_path;
struct imm_seq;

struct match_iter
{
  struct imm_seq const *seq;
  struct imm_path const *path;
  struct match match;
  unsigned idx;
  unsigned offset;
};

void match_iter_init(struct match_iter *, struct protein const *,
                     struct imm_seq const *, struct imm_path const *);
int match_iter_next(struct match_iter *);
bool match_iter_end(struct match_iter const *);
struct match const *match_iter_get(struct match_iter *);

#endif
