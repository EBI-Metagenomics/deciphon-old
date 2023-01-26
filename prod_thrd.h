#ifndef PROD_THRD_H
#define PROD_THRD_H

#include "prod.h"
#include <stdio.h>

struct match;

struct prod_thread
{
  FILE *fp;
  struct prod prod;
};

void prod_thrd_init(struct prod_thread *, FILE *);
int prod_thrd_write_begin(struct prod_thread *, struct prod const *);
int prod_thrd_write_match(struct prod_thread *, struct match const *);
int prod_thrd_write_sep(struct prod_thread *);
int prod_thrd_write_end(struct prod_thread *);

#endif
