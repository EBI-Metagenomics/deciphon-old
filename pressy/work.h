#ifndef WORK_H
#define WORK_H

#include "state.h"

void work_init(void);
enum state work_state(void);
int work_progress(void);
int work_run(char const *hmm, char const *db);
char const *work_hmmfile(void);
char const *work_dbfile(void);
void work_cancel(void);

#endif
