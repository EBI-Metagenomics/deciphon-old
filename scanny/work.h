#ifndef SCANNY_WORK_H
#define SCANNY_WORK_H

#include "state.h"
#include <stdbool.h>

void work_init(void);
enum state work_state(void);
int work_progress(void);
int work_run(char const *seqs, char const *db, bool multi_hits,
             bool hmmer3_compat);
char const *work_seqsfile(void);
void work_cancel(void);

#endif
