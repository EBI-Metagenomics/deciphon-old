#ifndef DECIPHON_API_H
#define DECIPHON_API_H

#include "deciphon/core/limits.h"
#include "deciphon/core/rc.h"
#include "sched_structs.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

struct api_error
{
    int rc;
    char msg[SCHED_JOB_ERROR_SIZE];
};

enum rc api_init(char const *url_stem, char const *api_key);
void api_cleanup(void);
struct api_error const *api_error(void);

bool api_is_reachable(void);

enum rc api_wipe(void);

enum rc api_hmm_up(char const *filepath, struct sched_hmm *);
enum rc api_hmm_dl(int64_t id, FILE *fp);
enum rc api_hmm_get_by_id(int64_t id, struct sched_hmm *);
enum rc api_hmm_get_by_xxh3(int64_t xxh3, struct sched_hmm *);
enum rc api_hmm_get_by_job_id(int64_t job_id, struct sched_hmm *);
enum rc api_hmm_get_by_filename(char const *filename, struct sched_hmm *);

enum rc api_db_up(char const *filepath, struct sched_db *);
enum rc api_db_dl(int64_t id, FILE *fp);
enum rc api_db_get_by_id(int64_t id, struct sched_db *);
enum rc api_db_get_by_xxh3(int64_t xxh3, struct sched_db *);
enum rc api_db_get_by_hmm_id(int64_t job_id, struct sched_db *);
enum rc api_db_get_by_filename(char const *filename, struct sched_db *);

enum rc api_job_next_pend(struct sched_job *job);
enum rc api_job_set_state(int64_t job_id, enum sched_job_state, char const *);
enum rc api_job_inc_progress(int64_t job_id, int increment);

enum rc api_scan_dl_seqs(int64_t id, FILE *fp);
enum rc api_scan_get_by_id(int64_t id, struct sched_scan *);
enum rc api_scan_get_by_job_id(int64_t job_id, struct sched_scan *);
enum rc api_scan_next_seq(int64_t scan_id, int64_t seq_id, struct sched_seq *);
enum rc api_scan_seq_count(int64_t scan_id, unsigned *count);
enum rc api_scan_submit(int64_t db_id, bool multi_hits, bool hmmer3_compat,
                        char const *filepath, struct sched_job *);

enum rc api_prods_file_up(char const *filepath);

#endif
