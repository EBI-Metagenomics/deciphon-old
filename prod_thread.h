#ifndef PROD_THREAD_H
#define PROD_THREAD_H

#include "prod.h"
#include <stdio.h>

struct match;

struct prod_thread
{
  FILE *fp;
  struct prod prod;
};

void prod_thread_init(struct prod_thread *, FILE *);
int prod_thread_write_begin(struct prod_thread *, struct prod const *);
int prod_thread_write_match(struct prod_thread *, struct match const *);
int prod_thread_write_sep(struct prod_thread *);
int prod_thread_write_end(struct prod_thread *);

#endif
