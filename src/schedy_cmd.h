#ifndef SCHEDY_CMD
#define SCHEDY_CMD

struct getcmd;

typedef void schedy_cmd_fn_t(struct getcmd const *);

void schedy_cmd_invalid(struct getcmd const *);
void schedy_cmd_init(struct getcmd const *);
void schedy_cmd_is_reachable(struct getcmd const *);
void schedy_cmd_wipe(struct getcmd const *);
void schedy_cmd_upload_hmm(struct getcmd const *);
void schedy_cmd_get_hmm(struct getcmd const *);
void schedy_cmd_download_hmm(struct getcmd const *);

#endif
