#ifndef SCHEDY_CMD
#define SCHEDY_CMD

struct getcmd;

typedef void schedy_cmd_fn_t(struct getcmd const *);

void schedy_cmd_invalid(struct getcmd const *);
void schedy_cmd_init(struct getcmd const *);
void schedy_cmd_is_reachable(struct getcmd const *);
void schedy_cmd_wipe(struct getcmd const *);
void schedy_cmd_upload_hmm(struct getcmd const *);
void schedy_cmd_get_hmm_by_id(struct getcmd const *);
void schedy_cmd_get_hmm_by_xxh3(struct getcmd const *);
void schedy_cmd_get_hmm_by_job_id(struct getcmd const *);
void schedy_cmd_get_hmm_by_filename(struct getcmd const *);
void schedy_cmd_download_hmm(struct getcmd const *);
void schedy_cmd_upload_db(struct getcmd const *);
void schedy_cmd_get_db(struct getcmd const *);
void schedy_cmd_next_pend_job(struct getcmd const *);
void schedy_cmd_set_job_state(struct getcmd const *);
void schedy_cmd_download_db(struct getcmd const *);
void schedy_cmd_upload_prods_file(struct getcmd const *);
void schedy_cmd_scan_next_seq(struct getcmd const *);
void schedy_cmd_scan_num_seqs(struct getcmd const *);
void schedy_cmd_get_scan_by_job_id(struct getcmd const *);
void schedy_cmd_increment_job_progress(struct getcmd const *);

#endif
