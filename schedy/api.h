#ifndef SCHEDY_API_H
#define SCHEDY_API_H

#include "deciphon_limits.h"
#include "rc.h"
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
enum rc api_hmm_dl(long hmm_id, FILE *fp);
enum rc api_hmm_get_by_id(long hmm_id, struct sched_hmm *);
enum rc api_hmm_get_by_xxh3(long xxh3, struct sched_hmm *);
enum rc api_hmm_get_by_job_id(long job_id, struct sched_hmm *);
enum rc api_hmm_get_by_filename(char const *filename, struct sched_hmm *);

enum rc api_db_up(char const *filepath, struct sched_db *);
enum rc api_db_dl(long db_id, FILE *fp);
enum rc api_db_get_by_id(long db_id, struct sched_db *);
enum rc api_db_get_by_xxh3(long xxh3, struct sched_db *);
enum rc api_db_get_by_hmm_id(long job_id, struct sched_db *);
enum rc api_db_get_by_filename(char const *filename, struct sched_db *);

enum rc api_job_get_by_id(long job_id, struct sched_job *);
enum rc api_job_inc_progress(long job_id, int increment);
enum rc api_job_next_pend(struct sched_job *);
enum rc api_job_set_state(long job_id, enum sched_job_state, char const *);

enum rc api_scan_dl_seqs(long scan_id, FILE *fp);
enum rc api_scan_get_by_id(long scan_id, struct sched_scan *);
enum rc api_scan_get_by_job_id(long job_id, struct sched_scan *);
enum rc api_scan_next_seq(long scan_id, long seq_id, struct sched_seq *);
enum rc api_scan_seq_count(long scan_id, unsigned *count);
enum rc api_scan_submit(long db_id, bool multi_hits, bool hmmer3_compat,
                        char const *filepath, struct sched_job *);

enum rc api_prods_file_up(char const *filepath);
enum rc api_prods_file_dl(long scan_id, FILE *fp);

#endif
