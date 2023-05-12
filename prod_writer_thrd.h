#ifndef PROD_WRITER_THRD_H
#define PROD_WRITER_THRD_H

#include "prod_match.h"
#include <stdio.h>

struct match;
struct match_iter;
struct hmmer_result;

struct prod_writer_thrd
{
  int idx;
  char const *dirname;
  char prodname[DCP_SHORT_PATH_MAX];
  struct prod_match match;
};

int prod_writer_thrd_init(struct prod_writer_thrd *, int idx,
                          char const *dirname);
int prod_writer_thrd_put(struct prod_writer_thrd *, struct match *,
                         struct match_iter *);
int prod_writer_thrd_put_hmmer(struct prod_writer_thrd *,
                               struct hmmer_result const *);

#endif
