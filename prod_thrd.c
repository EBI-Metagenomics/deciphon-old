#include "prod_thrd.h"
#include "dbl_fmt.h"
#include "deciphon/errno.h"
#include "match.h"
#include "protein.h"
#include "state.h"
#include <string.h>

/* Output example for two matches.
 *
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

void prod_thrd_init(struct prod_thrd *x, FILE *fp)
{
  x->fp = fp;
  prod_init(&x->prod);
}

int prod_thrd_write_begin(struct prod_thrd *x, struct prod const *y)
{
  FILE *fp = x->fp;
  if (fprintf(fp, "%ld\t", y->scan_id) < 0) return DCP_EWRITEPROD;
  if (fprintf(fp, "%ld\t", y->seq_id) < 0) return DCP_EWRITEPROD;

  if (fprintf(fp, "%s\t", y->protein) < 0) return DCP_EWRITEPROD;
  if (fprintf(fp, "%s\t", y->abc) < 0) return DCP_EWRITEPROD;

  if (fprintf(fp, DBL_FMT "\t", y->alt_loglik) < 0) return DCP_EWRITEPROD;
  if (fprintf(fp, DBL_FMT "\t", y->null_loglik) < 0) return DCP_EWRITEPROD;
  if (fprintf(fp, DBL_FMT "\t", y->evalue_log) < 0) return DCP_EWRITEPROD;

  if (fprintf(fp, "%s\t", y->version) < 0) return DCP_EWRITEPROD;

  return 0;
}

int prod_thrd_write_match(struct prod_thrd *x, struct match const *m)
{
  char buff[IMM_STATE_NAME_SIZE + 20] = {0};

  char *ptr = buff;
  memcpy(ptr, m->seq.str, m->seq.size);
  ptr += m->seq.size;
  *ptr++ = ',';

  m->protein->state_name(m->step.state_id, ptr);
  ptr += strlen(ptr);
  *ptr++ = ',';

  bool mute = state_is_mute(m->step.state_id);
  struct imm_codon codon = imm_codon_any(m->protein->nuclt_code->nuclt);
  if (!mute)
  {
    *ptr++ = imm_codon_asym(&codon);
    *ptr++ = imm_codon_bsym(&codon);
    *ptr++ = imm_codon_csym(&codon);
  }

  *ptr++ = ',';

  if (!mute) *ptr++ = imm_gc_decode(1, codon);

  *ptr = '\0';

  return fputs(buff, x->fp) == EOF ? DCP_EFWRITE : 0;
}

int prod_thrd_write_sep(struct prod_thrd *x)
{
  return fputc(';', x->fp) == EOF ? DCP_EWRITEPROD : 0;
}

int prod_thrd_write_end(struct prod_thrd *x)
{
  return fputc('\n', x->fp) == EOF ? DCP_EWRITEPROD : 0;
}
