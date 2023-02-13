#include "prod_match.h"
#include "lrt.h"
#include "sizeof_field.h"
#include <string.h>

void prod_match_init(struct prod_match *x)
{
  x->id = 0;
  x->seq_id = 0;

  memset(x->protein, 0, sizeof_field(struct prod_match, protein));
  memset(x->abc, 0, sizeof_field(struct prod_match, abc));

  x->alt = 0;
  x->null = 0;
  x->evalue = 0;
}

void prod_match_set_protein(struct prod_match *x, char const *protein)
{
  strcpy(x->protein, protein);
}

void prod_match_set_abc(struct prod_match *x, char const *abc)
{
  strcpy(x->abc, abc);
}

double prod_match_get_lrt(struct prod_match const *x)
{
  return lrt(x->null, x->alt);
}
