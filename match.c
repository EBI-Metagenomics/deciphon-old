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
    int rc = protein_decode(match->protein, &seq, step.state_id, &codon);
    if (rc) return rc;
  }
  return 0;
}

/* Match example
 *             ___________________________
 *             |   match0   |   match1   |
 *             ---------------------------
 * Output----->| CG,M1,CGA,K;CG,M4,CGA,K |
 *             ---|-|---|--|--------------
 * -----------   /  |   |  \    ---------------
 * | matched |__/   |   |   \___| most likely |
 * | letters |      |   |       | amino acid  |
 * -----------      |   |       ---------------
 *      -------------   ---------------
 *      | hmm state |   | most likely |
 *      -------------   | codon       |
 *                      ---------------
 */

int match_write(struct match const *x, FILE *fp)
{
  char buff[IMM_STATE_NAME_SIZE + 20] = {0};

  char *ptr = buff;
  memcpy(ptr, x->seq.str, x->seq.size);
  ptr += x->seq.size;
  *ptr++ = ',';

  x->protein->state_name(x->step.state_id, ptr);
  ptr += strlen(ptr);
  *ptr++ = ',';

  bool mute = state_is_mute(x->step.state_id);
  struct imm_codon codon = imm_codon_any(x->protein->nuclt_code->nuclt);
  if (!mute)
  {
    *ptr++ = imm_codon_asym(&codon);
    *ptr++ = imm_codon_bsym(&codon);
    *ptr++ = imm_codon_csym(&codon);
  }

  *ptr++ = ',';

  if (!mute) *ptr++ = imm_gc_decode(1, codon);

  *ptr = '\0';

  return fputs(buff, fp) == EOF ? DCP_EFWRITE : 0;
}
