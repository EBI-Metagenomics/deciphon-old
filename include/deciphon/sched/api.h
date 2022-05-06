#ifndef DECIPHON_SERVER_API_H
#define DECIPHON_SERVER_API_H

#include "deciphon/core/limits.h"
#include "deciphon/core/rc.h"
#include "deciphon/sched/sched.h"
#include <stdint.h>
#include <stdio.h>

struct api_rc
{
    int rc;
    char msg[SCHED_JOB_ERROR_SIZE];
};

enum sched_job_state;
struct sched_db;
struct sched_hmm;
struct sched_job;
struct sched_seq;

enum rc api_init(char const *url_stem, char const *api_key);
void api_cleanup(void);

bool api_is_reachable(void);

enum rc api_wipe(void);

enum rc api_upload_hmm(char const *filepath, struct sched_hmm *,
                       struct api_rc *);
enum rc api_get_hmm(int64_t id, struct sched_hmm *, struct api_rc *);
enum rc api_get_hmm_by_job_id(int64_t job_id, struct sched_hmm *,
                              struct api_rc *);
enum rc api_download_hmm(int64_t id, FILE *fp, struct api_rc *api_rc);

enum rc api_upload_db(char const *filepath, struct sched_db *, struct api_rc *);
enum rc api_get_db(int64_t id, struct sched_db *, struct api_rc *);

enum rc api_next_pend_job(struct sched_job *job, struct api_rc *);
enum rc api_set_job_state(int64_t job_id, enum sched_job_state state,
                          char const *msg, struct api_rc *);
enum rc api_download_db(int64_t id, FILE *fp, struct api_rc *api_rc);
enum rc api_upload_prods_file(char const *filepath, struct api_rc *);

enum rc api_scan_next_seq(int64_t scan_id, int64_t seq_id,
                          struct sched_seq *seq, struct api_rc *);
enum rc api_scan_num_seqs(int64_t scan_id, unsigned *num_seqs, struct api_rc *);

enum rc api_get_scan_by_job_id(int64_t job_id, struct sched_scan *,
                               struct api_rc *);

enum rc api_increment_job_progress(int64_t job_id, int increment,
                                   struct api_rc *api_rc);

#endif
