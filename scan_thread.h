#ifndef SCAN_THREAD_H
#define SCAN_THREAD_H

#include "prod.h"
#include "scan_task.h"
#include <stdio.h>

struct scan_thread
{
  struct protein_iter *iter;

  struct imm_seq const *seq;

  double lrt_threshold;
  bool multi_hits;
  bool hmmer3_compat;

  struct prod prod;
  struct scan_task null;
  struct scan_task alt;
  struct hmmer_result *hmmer_result;
  char *amino;

  // struct progress progress;
};

enum imm_abc_typeid;
enum profile_typeid;

void scan_thread_init(struct scan_thread *, struct protein_iter *);
void scan_thread_set_sequence(struct scan_thread *, struct imm_seq const *);
int scan_thread_run(struct scan_thread *);

#endif
