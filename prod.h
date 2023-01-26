#ifndef PROD_H
#define PROD_H

#include "deciphon/limits.h"
#include <stdio.h>

struct prod
{
  long id;

  long scan_id;
  long seq_id;

  char protein[DCP_PROFILE_NAME_SIZE];
  char abc[DCP_ABC_NAME_SIZE];

  double alt_loglik;
  double null_loglik;
  double evalue_log;

  char version[DCP_VERSION_SIZE];
};

void prod_init(struct prod *);
void prod_set_protein(struct prod *, char const *);
void prod_set_abc(struct prod *, char const *);
void prod_set_scan_id(struct prod *, long scan_id);
void prod_set_seq_id(struct prod *, long seq_id);

void prod_set_null_loglik(struct prod *, double);
void prod_set_alt_loglik(struct prod *, double);

double prod_get_lrt(struct prod const *);

#endif
