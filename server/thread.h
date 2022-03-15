#ifndef THREAD_H
#define THREAD_H

#include "hypothesis.h"
#include "prod.h"
#include "protein_match.h"

struct thread
{
    unsigned id;

    struct imm_seq *iseq;
    struct profile_reader *reader;

    bool multi_hits;
    bool hmmer3_compat;
    imm_float lrt_threshold;

    struct prod prod;
    struct hypothesis null;
    struct hypothesis alt;
    union
    {
        // struct standard_match std;
        struct protein_match pro;
    } match;

    prod_fwrite_match_func_t *write_match_func;
};

enum imm_abc_typeid;
enum profile_typeid;

void thread_init(struct thread *, unsigned id, struct profile_reader *reader,
                 bool multi_hits, bool hmmer3_compat, imm_float lrt_threshold);
void thread_setup_job(struct thread *, enum imm_abc_typeid, enum profile_typeid,
                      unsigned job_id);
void thread_setup_seq(struct thread *, unsigned seq_id);
enum rc thread_run(struct thread *, struct imm_seq *seq);

#endif
