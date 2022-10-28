#ifndef SCANNY_SCANNER_H
#define SCANNY_SCANNER_H

#include "core/rc.h"
#include <stdbool.h>

void scanner_init(void);
void scanner_reset(void);
char const *scanner_filename(void);
void scanner_set_nthreads(int nthreads);
bool scanner_is_running(void);
bool scanner_is_done(void);
bool scanner_start(char const *seqs, char const *db, char const *prod,
                   bool multi_hits, bool hmmer3_compat);
int scanner_progress(void);
int scanner_cancel(int timeout_msec);
char const *scanner_state_string(void);

#endif
