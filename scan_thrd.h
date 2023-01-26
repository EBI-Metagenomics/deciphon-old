#ifndef SCAN_THRD_H
#define SCAN_THRD_H

#include "prod.h"
#include "protein_iter.h"
#include "scan_task.h"
#include <stdio.h>

struct scan_thread
{
  struct protein protein;
  struct protein_iter iter;

  double lrt_threshold;
  bool multi_hits;
  bool hmmer3_compat;

  struct prod prod;
  struct hmmer_result *hmmer_result;
  char *amino;

  // struct progress progress;
};

struct prod_thread;
struct protein_reader;

void scan_thrd_init(struct scan_thread *, struct protein_reader *,
                    int partition);
int scan_thrd_run(struct scan_thread *, struct imm_seq const *,
                  struct prod_thread *);
void scan_thrd_cleanup(struct scan_thread *);

#endif
