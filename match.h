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
  struct imm_codon codon;
};

void match_init(struct match *, struct protein const *);
int match_setup(struct match *, struct imm_step, struct imm_seq);
void match_state_name(struct match const *, char *dst);
bool match_state_is_mute(struct match const *);
char match_amino(struct match const *);
struct imm_codon match_codon(struct match const *);

#endif
