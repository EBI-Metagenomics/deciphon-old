#ifndef SCHEDY_CMD
#define SCHEDY_CMD

struct getcmd;

typedef void schedy_cmd_fn_t(struct getcmd const *);

void schedy_cmd_invalid(struct getcmd const *);
void schedy_cmd_connect(struct getcmd const *);
void schedy_cmd_online(struct getcmd const *);
void schedy_cmd_disconnect(struct getcmd const *);
void schedy_cmd_wipe(struct getcmd const *);

void schedy_cmd_hmm_up(struct getcmd const *);
void schedy_cmd_hmm_dl(struct getcmd const *);
void schedy_cmd_hmm_get_by_id(struct getcmd const *);
void schedy_cmd_hmm_get_by_xxh3(struct getcmd const *);
void schedy_cmd_hmm_get_by_job_id(struct getcmd const *);
void schedy_cmd_hmm_get_by_filename(struct getcmd const *);

void schedy_cmd_db_up(struct getcmd const *);
void schedy_cmd_db_dl(struct getcmd const *);
void schedy_cmd_db_get_by_id(struct getcmd const *);
void schedy_cmd_db_get_by_xxh3(struct getcmd const *);
void schedy_cmd_db_get_by_job_id(struct getcmd const *);
void schedy_cmd_db_get_by_filename(struct getcmd const *);

void schedy_cmd_job_next_pend(struct getcmd const *);
void schedy_cmd_job_set_state(struct getcmd const *);
void schedy_cmd_job_inc_progress(struct getcmd const *);

void schedy_cmd_prods_file_up(struct getcmd const *);

void schedy_cmd_scan_next_seq(struct getcmd const *);
void schedy_cmd_scan_num_seqs(struct getcmd const *);
void schedy_cmd_scan_get_by_job_id(struct getcmd const *);

#endif
