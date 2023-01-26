#ifndef SCAN_THREAD_H
#define SCAN_THREAD_H

#include "prod.h"
#include "protein_iter.h"
#include "scan_task.h"
#include <stdio.h>

struct scan_thread
{
  struct protein_iter iter;

  double lrt_threshold;
  bool multi_hits;
  bool hmmer3_compat;

  struct prod prod;
  struct hmmer_result *hmmer_result;
  char *amino;

  // struct progress progress;
};

struct protein_reader;

void scan_thread_init(struct scan_thread *, struct protein_reader *,
                      int partition);
int scan_thread_run(struct scan_thread *, struct imm_seq const *);

#endif
