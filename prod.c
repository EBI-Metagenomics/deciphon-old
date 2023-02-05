#include "prod.h"
#include "dbl_fmt.h"
#include "deciphon/errno.h"
#include "lrt.h"
#include "sizeof_field.h"
#include <string.h>

void prod_init(struct prod *x)
{
  x->id = 0;
  x->scan_id = 0;
  x->seq_id = 0;

  memset(x->protein, 0, sizeof_field(struct prod, protein));
  memset(x->abc, 0, sizeof_field(struct prod, abc));

  x->alt_loglik = 0;
  x->null_loglik = 0;
  x->evalue_log = 0;
}

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
