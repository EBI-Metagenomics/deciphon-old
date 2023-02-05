#include "prod_thrd.h"
#include "dbl_fmt.h"
#include "deciphon/errno.h"
#include "fs.h"
#include "hmmer_result.h"
#include "match.h"
#include "match_iter.h"
#include "prod_thrd.h"
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

int write_begin(struct prod_thrd *, struct prod const *);
int write_match(struct prod_thrd *, struct match const *);
int write_sep(struct prod_thrd *);
int write_end(struct prod_thrd *);

int prod_thrd_write(struct prod_thrd *x, struct prod const *prod,
                    struct match *match, struct match_iter *it)
{
  int rc = 0;
  if ((rc = write_begin(x, prod))) return rc;

  int i = 0;
  while (!(rc = match_iter_next(it, match)))
  {
    if (match_iter_end(it)) break;
    if (i++ && (rc = write_sep(x))) return rc;
    if ((rc = write_match(x, match))) return rc;
  }

  return write_end(x);
}

int prod_thrd_write_hmmer(struct prod_thrd *x, struct prod const *prod,
                          struct hmmer_result const *result)
{
  char filename[128] = {0};
  int rc = 0;

  sprintf(filename, "prod/hmmer/%ld", prod->seq_id);
  if ((rc = fs_mkdir(filename, true))) return rc;
  sprintf(filename, "prod/hmmer/%ld/%s.h3r", prod->seq_id, prod->protein);

  FILE *fp = fopen(filename, "wb");
  if (!fp) return DCP_EFOPEN;

  if ((rc = hmmer_result_pack(result, fp)))
  {
    fclose(fp);
    return rc;
  }

  return fclose(fp) ? DCP_EFCLOSE : 0;
}

int write_begin(struct prod_thrd *x, struct prod const *y)
{
  FILE *fp = x->fp;
  if (fprintf(fp, "%ld\t", y->scan_id) < 0) return DCP_EWRITEPROD;
  if (fprintf(fp, "%ld\t", y->seq_id) < 0) return DCP_EWRITEPROD;

  if (fprintf(fp, "%s\t", y->protein) < 0) return DCP_EWRITEPROD;
  if (fprintf(fp, "%s\t", y->abc) < 0) return DCP_EWRITEPROD;

  if (fprintf(fp, DBL_FMT "\t", y->alt_loglik) < 0) return DCP_EWRITEPROD;
  if (fprintf(fp, DBL_FMT "\t", y->null_loglik) < 0) return DCP_EWRITEPROD;
  if (fprintf(fp, DBL_FMT "\t", y->evalue_log) < 0) return DCP_EWRITEPROD;

  return 0;
}

int write_match(struct prod_thrd *x, struct match const *m)
{
  char buff[IMM_STATE_NAME_SIZE + 20] = {0};

  char *ptr = buff;
  memcpy(ptr, m->seq.str, m->seq.size);
  ptr += m->seq.size;
  *ptr++ = ',';

  match_state_name(m, ptr);
  ptr += strlen(ptr);
  *ptr++ = ',';

  if (!match_state_is_mute(m))
  {
    struct imm_codon codon = match_codon(m);
    *ptr++ = imm_codon_asym(&codon);
    *ptr++ = imm_codon_bsym(&codon);
    *ptr++ = imm_codon_csym(&codon);
  }

  *ptr++ = ',';

  if (!match_state_is_mute(m)) *ptr++ = match_amino(m);

  *ptr = '\0';

  return fputs(buff, x->fp) == EOF ? DCP_EFWRITE : 0;
}

int write_sep(struct prod_thrd *x)
{
  return fputc(';', x->fp) == EOF ? DCP_EWRITEPROD : 0;
}

int write_end(struct prod_thrd *x)
{
  return fputc('\n', x->fp) == EOF ? DCP_EWRITEPROD : 0;
}
