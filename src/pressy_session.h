#ifndef PRESSY_SESSION_H
#define PRESSY_SESSION_H

#include <stdbool.h>

struct uv_loop_s;

void pressy_session_init(struct uv_loop_s *loop);
bool pressy_session_is_running(void);
bool pressy_session_is_done(void);
bool pressy_session_start(char const *hmm_filepath);
unsigned pressy_session_progress(void);
bool pressy_session_cancel(void);
char const *pressy_session_state_string(void);

#endif
