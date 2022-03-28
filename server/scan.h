#ifndef SCAN_H
#define SCAN_H

#include "deciphon/db/db.h"
#include "deciphon/server/sched.h"
#include "scan_thread.h"

struct scan
{
    struct sched_scan sched;
    struct sched_seq seq;

    unsigned num_threads;
    struct scan_thread thread[NUM_THREADS];

    char db_filename[FILENAME_SIZE];
    struct protein_db_reader db_reader;
    struct profile_reader profile_reader;

    struct imm_str str;
    struct imm_seq iseq;
    struct imm_abc const *abc;

    prod_fwrite_match_func_t write_match_func;
    double lrt_threshold;
};

enum rc;

enum rc scan_init(struct scan *, unsigned num_threads, double lrt_threshold);
enum rc scan_run(struct scan *);
enum rc scan_cleanup(struct scan *);

#endif
