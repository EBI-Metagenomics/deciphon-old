#ifndef PROD_FILE_H
#define PROD_FILE_H

#include "deciphon/limits.h"
#include "prod_thrd.h"
#include <stdio.h>

struct match;

struct prod_file
{
  int size;
  FILE *files[DCP_NTHREADS_MAX];
  struct prod_thrd prod_threads[DCP_NTHREADS_MAX];
};

int prod_file_setup(struct prod_file *, int size);
struct prod_thrd *prod_file_thread(struct prod_file *, int idx);
int prod_file_finishup(struct prod_file *, FILE *);
void prod_file_cleanup(struct prod_file *);

#endif
