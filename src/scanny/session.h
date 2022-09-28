#ifndef SCANNY_SESSION_H
#define SCANNY_SESSION_H

#include <stdbool.h>

struct uv_loop_s;

void session_init(struct uv_loop_s *loop);
void session_set_nthreads(int num_threads);
bool session_is_running(void);
bool session_is_done(void);
bool session_start(char const *seqs, char const *db, char const *prod,
                   bool multi_hits, bool hmmer3_compat);
int session_progress(void);
bool session_cancel(void);
char const *session_state_string(void);

#endif
