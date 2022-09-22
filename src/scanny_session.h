#ifndef SCANNY_SESSION_H
#define SCANNY_SESSION_H

#include <stdbool.h>

struct uv_loop_s;

void scanny_session_init(struct uv_loop_s *loop);
bool scanny_session_is_running(void);
bool scanny_session_is_done(void);
bool scanny_session_start(char const *seqs, char const *db, bool multi_hits,
                          bool hmmer3_compat);
unsigned scanny_session_progress(void);
bool scanny_session_cancel(void);
char const *scanny_session_state_string(void);

#endif