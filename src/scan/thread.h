#ifndef SCAN_THREAD_H
#define SCAN_THREAD_H

#include "scan/cfg.h"
#include "scan/hypothesis.h"
#include "scan/prod.h"
#include "scan/protein_match.h"

struct scan_thread
{
    unsigned id;
    int64_t job_id;

    struct imm_seq const *seq;
    struct profile_reader *reader;

    struct scan_cfg cfg;

    struct prod prod;
    struct hypothesis null;
    struct hypothesis alt;
    union
    {
        // struct standard_match std;
        struct protein_match pro;
    } match;

    prod_fwrite_match_func_t write_match_func;
};

enum imm_abc_typeid;
enum profile_typeid;

void thread_init(struct scan_thread *, unsigned id,
                 struct profile_reader *reader, struct scan_cfg cfg,
                 prod_fwrite_match_func_t write_match_func);
void thread_setup_job(struct scan_thread *, enum imm_abc_typeid,
                      enum profile_typeid, int64_t scan_id);
void thread_setup_seq(struct scan_thread *, struct imm_seq *seq,
                      int64_t seq_id);
enum rc thread_run(struct scan_thread *, int tid);

#endif
