#ifndef HMMY_HMMER_H
#define HMMY_HMMER_H

#include "core/rc.h"
#include <stdbool.h>

void hmmer_init(void);
void hmmer_reset(void);
bool hmmer_is_running(void);
bool hmmer_is_done(void);
bool hmmer_start(char const *filename);
bool hmmer_stop(char const *filename);
char const *hmmer_filename(void);
int hmmer_cancel(int timeout_msec);
char const *hmmer_state_string(void);

#endif
