#include "match.h"
#include "protein.h"
#include "state.h"
#include <string.h>

void match_init(struct match *x, struct protein const *protein)
{
  x->protein = protein;
}

int match_setup(struct match *x, struct imm_step step, struct imm_seq seq)
{
  x->step = step;
  x->seq = seq;

  if (!state_is_mute(step.state_id))
  {
    x->codon = imm_codon_any(x->protein->nuclt_code->nuclt);
    return protein_decode(x->protein, &seq, step.state_id, &x->codon);
  }
  return 0;
}

void match_state_name(struct match const *x, char *dst)
{
  x->protein->state_name(x->step.state_id, dst);
}

bool match_state_is_mute(struct match const *x)
{
  return state_is_mute(x->step.state_id);
}

char match_amino(struct match const *x)
{
  return imm_gencode_decode(x->protein->gencode, x->codon);
}

struct imm_codon match_codon(struct match const *x) { return x->codon; }
