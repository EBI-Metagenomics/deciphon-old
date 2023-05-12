#include "prod_writer_thrd.h"
#include "array_size.h"
#include "array_size_field.h"
#include "dbl_fmt.h"
#include "deciphon/errno.h"
#include "defer_return.h"
#include "fmt.h"
#include "fs.h"
#include "hmmer_result.h"
#include "match.h"
#include "match_iter.h"
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

int prod_writer_thrd_init(struct prod_writer_thrd *x, int idx, char const *dir)
{
  int rc = 0;
  x->idx = idx;
  x->dirname = dir;
  size_t n = array_size_field(struct prod_writer_thrd, prodname);
  if ((rc = fmt(x->prodname, n, "%s/.products.%03d.tsv", dir, idx))) return rc;
  if ((rc = fs_touch(x->prodname))) return rc;
  prod_match_init(&x->match);
  return 0;
}

static int write_begin(FILE *, struct prod_match const *);
static int write_match(FILE *, struct match const *);
static int write_sep(FILE *);
static int write_end(FILE *);

int prod_writer_thrd_put(struct prod_writer_thrd *x, struct match *match,
                         struct match_iter *it)
{
  int rc = 0;

  FILE *fp = fopen(x->prodname, "ab");
  if (!fp) return DCP_EFOPEN;

  if ((rc = write_begin(fp, &x->match))) defer_return(rc);

  int i = 0;
  while (!(rc = match_iter_next(it, match)))
  {
    if (match_iter_end(it)) break;
    if (i++ && (rc = write_sep(fp))) defer_return(rc);
    if ((rc = write_match(fp, match))) defer_return(rc);
  }

  if ((rc = write_end(fp))) defer_return(rc);

  return fclose(fp) ? DCP_EFCLOSE : 0;

defer:
  fclose(fp);
  return rc;
}

int prod_writer_thrd_put_hmmer(struct prod_writer_thrd *x,
                               struct hmmer_result const *result)
{
  char file[DCP_SHORT_PATH_MAX] = {0};
  int rc = 0;
  char const *dirname = x->dirname;

  if ((rc = FMT(file, "%s/hmmer/%ld", dirname, x->match.seq_id))) return rc;
  if ((rc = fs_mkdir(file, true))) return rc;

  if ((rc = FMT(file, "%s/hmmer/%ld/%s.h3r", dirname, x->match.seq_id,
                x->match.protein)))
    return rc;

  FILE *fp = fopen(file, "wb");
  if (!fp) return DCP_EFOPEN;

  if ((rc = hmmer_result_pack(result, fp)))
  {
    fclose(fp);
    return rc;
  }

  return fclose(fp) ? DCP_EFCLOSE : 0;
}

static int write_begin(FILE *fp, struct prod_match const *y)
{
  if (fprintf(fp, "%ld\t", y->seq_id) < 0) return DCP_EWRITEPROD;

  if (fprintf(fp, "%s\t", y->protein) < 0) return DCP_EWRITEPROD;
  if (fprintf(fp, "%s\t", y->abc) < 0) return DCP_EWRITEPROD;

  if (fprintf(fp, DBL_FMT "\t", y->alt) < 0) return DCP_EWRITEPROD;
  if (fprintf(fp, DBL_FMT "\t", y->null) < 0) return DCP_EWRITEPROD;
  if (fprintf(fp, DBL_FMT "\t", y->evalue) < 0) return DCP_EWRITEPROD;

  return 0;
}

static int write_match(FILE *fp, struct match const *m)
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

  return fputs(buff, fp) == EOF ? DCP_EFWRITE : 0;
}

static int write_sep(FILE *fp)
{
  return fputc(';', fp) == EOF ? DCP_EWRITEPROD : 0;
}

static int write_end(FILE *fp)
{
  return fputc('\n', fp) == EOF ? DCP_EWRITEPROD : 0;
}
