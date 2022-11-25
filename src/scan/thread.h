#ifndef SCAN_THREAD_H
#define SCAN_THREAD_H

#include "progress.h"
#include "scan/cfg.h"
#include "scan/hypothesis.h"
#include "scan/prod.h"
#include "scan/prot_result.h"
#include <stdio.h>

struct thread
{
    FILE *prodfile;
    int idx;
    int prof_start;

    struct imm_seq const *seq;
    struct profile_reader *reader;

    struct scan_cfg cfg;

    struct prod prod;
    struct hypothesis null;
    struct hypothesis alt;
    struct prot_result result;

    struct progress progress;

    enum rc errnum;
    char errmsg[ERROR_SIZE];
};

enum imm_abc_typeid;
enum profile_typeid;

void thread_init(struct thread *, FILE *prodfile, int idx, int prof_start,
                 struct profile_reader *, struct scan_cfg);
void thread_setup_job(struct thread *, enum imm_abc_typeid, enum profile_typeid,
                      long scan_id, long ntasks);
void thread_setup_seq(struct thread *, struct imm_seq const *, long seq_id);
enum rc thread_run(struct thread *);
struct progress const *thread_progress(struct thread const *);
char const *thread_errmsg(struct thread const *);

#endif
