#include "prod.h"
#include "dbl_fmt.h"
#include "deciphon/errno.h"
#include "deciphon/version.h"
#include "lrt.h"
#include <string.h>

void prod_init(struct prod *x) { strcpy(x->version, DCP_VERSION); }

void prod_set_protein(struct prod *x, char const *protein)
{
  strcpy(x->protein, protein);
}

void prod_set_abc(struct prod *x, char const *abc) { strcpy(x->abc, abc); }

void prod_set_scan_id(struct prod *x, long scan_id) { x->scan_id = scan_id; }

void prod_set_seq_id(struct prod *x, long seq_id) { x->seq_id = seq_id; }

void prod_set_null_loglik(struct prod *x, double loglik)
{
  x->null_loglik = loglik;
}

void prod_set_alt_loglik(struct prod *x, double loglik)
{
  x->alt_loglik = loglik;
}

double prod_get_lrt(struct prod const *x)
{
  return lrt(x->null_loglik, x->alt_loglik);
}

/* Output example
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
int prod_write_begin(struct prod const *x, FILE *fp)
{
  if (fprintf(fp, "%ld\t", x->scan_id) < 0) return DCP_EWRITEPROD;
  if (fprintf(fp, "%ld\t", x->seq_id) < 0) return DCP_EWRITEPROD;

  if (fprintf(fp, "%s\t", x->protein) < 0) return DCP_EWRITEPROD;
  if (fprintf(fp, "%s\t", x->abc) < 0) return DCP_EWRITEPROD;

  if (fprintf(fp, DBL_FMT "\t", x->alt_loglik) < 0) return DCP_EWRITEPROD;
  if (fprintf(fp, DBL_FMT "\t", x->null_loglik) < 0) return DCP_EWRITEPROD;
  if (fprintf(fp, DBL_FMT "\t", x->evalue_log) < 0) return DCP_EWRITEPROD;

  if (fprintf(fp, "%s\t", x->version) < 0) return DCP_EWRITEPROD;

  return 0;
}

int prod_write_sep(FILE *fp)
{
  return fputc(';', fp) == EOF ? DCP_EWRITEPROD : 0;
}

int prod_write_end(FILE *fp)
{
  return fputc('\n', fp) == EOF ? DCP_EWRITEPROD : 0;
}
