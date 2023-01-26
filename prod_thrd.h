#ifndef PROD_THRD_H
#define PROD_THRD_H

#include "prod.h"
#include <stdio.h>

struct match;
struct match_iter;

struct prod_thrd
{
  FILE *fp;
  struct prod prod;
};

void prod_thrd_init(struct prod_thrd *, FILE *);
int prod_thrd_write(struct prod_thrd *, struct prod const *, struct match *,
                    struct match_iter *);
int write_begin(struct prod_thrd *, struct prod const *);
int write_match(struct prod_thrd *, struct match const *);
int write_sep(struct prod_thrd *);
int write_end(struct prod_thrd *);

#endif
