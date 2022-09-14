#ifndef SCHEDY_CMD
#define SCHEDY_CMD

struct getcmd;

typedef char const *schedy_cmd_fn_t(struct getcmd const *);

char const *schedy_cmd_invalid(struct getcmd const *);
char const *schedy_cmd_connect(struct getcmd const *);
char const *schedy_cmd_online(struct getcmd const *);
char const *schedy_cmd_wipe(struct getcmd const *);

char const *schedy_cmd_hmm_up(struct getcmd const *);
char const *schedy_cmd_hmm_dl(struct getcmd const *);
char const *schedy_cmd_hmm_get_by_id(struct getcmd const *);
char const *schedy_cmd_hmm_get_by_xxh3(struct getcmd const *);
char const *schedy_cmd_hmm_get_by_job_id(struct getcmd const *);
char const *schedy_cmd_hmm_get_by_filename(struct getcmd const *);

char const *schedy_cmd_db_up(struct getcmd const *);
char const *schedy_cmd_db_dl(struct getcmd const *);
char const *schedy_cmd_db_get_by_id(struct getcmd const *);
char const *schedy_cmd_db_get_by_xxh3(struct getcmd const *);
char const *schedy_cmd_db_get_by_hmm_id(struct getcmd const *);
char const *schedy_cmd_db_get_by_filename(struct getcmd const *);

char const *schedy_cmd_job_next_pend(struct getcmd const *);
char const *schedy_cmd_job_set_state(struct getcmd const *);
char const *schedy_cmd_job_inc_progress(struct getcmd const *);

char const *schedy_cmd_scan_dl_seqs(struct getcmd const *);
char const *schedy_cmd_scan_get_by_job_id(struct getcmd const *);
char const *schedy_cmd_scan_next_seq(struct getcmd const *);
char const *schedy_cmd_scan_seq_count(struct getcmd const *);
char const *schedy_cmd_scan_submit(struct getcmd const *);

char const *schedy_cmd_prods_file_up(struct getcmd const *);

#endif
