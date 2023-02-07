#ifndef SCAN_THRD_H
#define SCAN_THRD_H

#include "chararray.h"
#include "hmmer.h"
#include "protein_iter.h"
#include "scan_task.h"
#include <stdio.h>

struct chararray;
struct prod_writer_thrd;

struct scan_thrd
{
  struct protein protein;
  struct protein_iter iter;

  double lrt_threshold;
  bool multi_hits;
  bool hmmer3_compat;

  struct prod_writer_thrd *prod_thrd;
  struct chararray amino;
  struct hmmer hmmer;
};

struct prod_thrd;
struct protein_reader;
struct hmmer_dialer;

int scan_thrd_init(struct scan_thrd *, struct protein_reader *, int partition,
                   long scan_id, struct prod_writer_thrd *,
                   struct hmmer_dialer *);
void scan_thrd_cleanup(struct scan_thrd *);

void scan_thrd_set_seq_id(struct scan_thrd *, long seq_id);
void scan_thrd_set_lrt_threshold(struct scan_thrd *, double lrt);
void scan_thrd_set_multi_hits(struct scan_thrd *, bool multihits);
void scan_thrd_set_hmmer3_compat(struct scan_thrd *, bool h3compat);
int scan_thrd_run(struct scan_thrd *, struct imm_seq const *);

#endif
