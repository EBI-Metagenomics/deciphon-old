#ifndef MATCH_H
#define MATCH_H

#include "imm/imm.h"
#include <stdbool.h>

struct protein;

struct match
{
  struct protein const *protein;
  struct imm_step step;
  struct imm_seq seq;
};

void match_init(struct match *, struct protein const *);
int match_setup(struct match *, struct imm_step, struct imm_seq);
int match_write(struct match const *, FILE *);

#endif
