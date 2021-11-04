#ifndef SCHED_H
#define SCHED_H

#include "dcp/job_state.h"
#include "dcp/rc.h"
#include "dcp/sched.h"
#include "dcp/seq.h"
#include "path.h"

struct dcp_job;
struct sqlite3;
struct sqlite3_stmt;

struct sched
{
    struct sqlite3 *db;
    struct
    {
        struct
        {
            struct sqlite3_stmt *job;
            struct sqlite3_stmt *seq;
        } submit;
        struct
        {
            struct sqlite3_stmt *state;
            struct sqlite3_stmt *pend;
        } job;
        struct
        {
            struct sqlite3_stmt *select;
            struct sqlite3_stmt *insert;
        } db;
        struct sqlite3_stmt *seq;
    } stmt;
};

enum dcp_rc sched_setup(char const *filepath);
enum dcp_rc sched_open(struct sched *, char const *filepath);
enum dcp_rc sched_close(struct sched *);

enum dcp_rc sched_submit_job(struct sched *, struct dcp_job *,
                             dcp_sched_id db_id, dcp_sched_id *job_id);

enum dcp_rc sched_add_db(struct sched *, char const *filepath,
                         dcp_sched_id *id);

enum dcp_rc sched_job_state(struct sched *, dcp_sched_id job_id,
                            enum dcp_job_state *);

enum dcp_rc sched_next_job(struct sched *, struct dcp_job *);

enum dcp_rc sched_db_filepath(struct sched *, dcp_sched_id,
                              char filepath[PATH_SIZE]);

enum dcp_rc sched_next_seq(struct sched *, dcp_sched_id job_id,
                           dcp_sched_id *seq_id, struct dcp_seq *seq);

enum dcp_rc sched_add_result(struct sched *, dcp_sched_id job_id,
                             char const *output, char const *codon,
                             char const *amino);

#endif
