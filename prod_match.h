#ifndef PROD_MATCH_H
#define PROD_MATCH_H

#include "deciphon/limits.h"

struct prod_match
{
  long id;

  long seq_id;

  char protein[DCP_PROFILE_NAME_SIZE];
  char abc[DCP_ABC_NAME_SIZE];

  double alt;
  double null;
  double evalue;
};

void prod_match_init(struct prod_match *);
void prod_match_set_protein(struct prod_match *, char const *);
void prod_match_set_abc(struct prod_match *, char const *);
double prod_match_get_lrt(struct prod_match const *);

#endif
