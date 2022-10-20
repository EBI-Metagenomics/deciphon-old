#ifndef PRESSY_PRESSER_H
#define PRESSY_PRESSER_H

#include "core/rc.h"
#include <stdbool.h>

void presser_init(void);
bool presser_is_running(void);
bool presser_is_done(void);
bool presser_start(char const *hmm_filepath);
int presser_progress(void);
int presser_cancel(int timeout_msec);
char const *presser_state_string(void);

#endif
