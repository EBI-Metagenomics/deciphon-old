#ifndef MATCH_ITER_H
#define MATCH_ITER_H

#include <stdbool.h>

struct imm_path;
struct imm_seq;
struct match;

struct match_iter
{
  struct imm_seq const *seq;
  struct imm_path const *path;
  int idx;
  int offset;
};

void match_iter_init(struct match_iter *, struct imm_seq const *,
                     struct imm_path const *);
int match_iter_next(struct match_iter *, struct match *);
bool match_iter_end(struct match_iter const *);

#endif
