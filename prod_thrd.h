#ifndef PROD_THRD_H
#define PROD_THRD_H

#include "prod.h"
#include <stdio.h>

struct match;
struct match_iter;
struct hmmer_result;

struct prod_thrd
{
  FILE *fp;
  struct prod prod;
};

void prod_thrd_init(struct prod_thrd *, FILE *);
int prod_thrd_write(struct prod_thrd *, struct prod const *, struct match *,
                    struct match_iter *);
int prod_thrd_write_hmmer(struct prod_thrd *, struct prod const *,
                          struct hmmer_result const *);

#endif
