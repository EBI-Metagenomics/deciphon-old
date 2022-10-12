#ifndef PRESSY_SESSION_H
#define PRESSY_SESSION_H

#include <stdbool.h>

void session_init(void);
bool session_is_running(void);
bool session_is_done(void);
bool session_start(char const *hmm_filepath);
unsigned session_progress(void);
bool session_cancel(void);
char const *session_state_string(void);

#endif
