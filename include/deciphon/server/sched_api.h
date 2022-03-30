#ifndef DECIPHON_SERVER_SCHED_API_H
#define DECIPHON_SERVER_SCHED_API_H

#include "deciphon/limits.h"
#include "deciphon/rc.h"
#include "deciphon/server/sched.h"
#include <stdint.h>
#include <stdio.h>

struct sched_api_error
{
    enum sched_rc rc;
    char msg[ERROR_SIZE];
};

static inline void sched_api_error_init(struct sched_api_error *err)
{
    err->rc = SCHED_OK;
    err->msg[0] = 0;
}

enum sched_job_state;
struct sched_db;
struct sched_hmm;
struct sched_job;
struct sched_seq;

enum rc sched_api_init(char const *url_stem);
void sched_api_cleanup(void);

enum rc sched_api_wipe(void);

enum rc sched_api_upload_hmm(char const *filepath, struct sched_hmm *,
                             struct sched_api_error *);
enum rc sched_api_get_hmm(int64_t id, struct sched_hmm *,
                          struct sched_api_error *);
enum rc sched_api_get_hmm_by_job_id(int64_t job_id, struct sched_hmm *,
                                    struct sched_api_error *);

enum rc sched_api_upload_db(char const *filepath, struct sched_db *,
                            struct sched_api_error *);
enum rc sched_api_add_db(struct sched_db *db, struct sched_api_error *);
enum rc sched_api_get_db(struct sched_db *db, struct sched_api_error *);

enum rc sched_api_post_testing_data(struct sched_api_error *);
enum rc sched_api_next_pend_job(struct sched_job *job,
                                struct sched_api_error *);
enum rc sched_api_scan_next_seq(int64_t scan_id, struct sched_seq *seq,
                                struct sched_api_error *);
enum rc sched_api_set_job_state(int64_t job_id, enum sched_job_state state,
                                char const *msg, struct sched_api_error *);
enum rc sched_api_download_db(struct sched_db *db, FILE *fp);
enum rc sched_api_upload_prods_file(char const *filepath,
                                    struct sched_api_error *);

#endif
