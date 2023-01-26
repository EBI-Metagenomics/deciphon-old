#include "match.h"
#include "protein.h"
#include "state.h"
#include <string.h>

void match_init(struct match *match, struct protein const *protein)
{
  match->protein = protein;
}

int match_setup(struct match *match, struct imm_step step, struct imm_seq seq)
{
  match->step = step;
  match->seq = seq;

  if (!state_is_mute(step.state_id))
  {
    struct imm_codon codon = imm_codon_any(match->protein->nuclt_code->nuclt);
    return protein_decode(match->protein, &seq, step.state_id, &codon);
  }
  return 0;
}
