#ifndef WORK_H
#define WORK_H

#include "state.h"

enum work_end_reason
{
    SUCCESS,
    FAILURE,
    CANCELLED,
};

typedef void work_end_fn_t(enum work_end_reason, char const *hmm);

void work_init(void);
enum state work_state(void);
int work_progress(void);
int work_run(char const *hmm, char const *db);
char const *work_hmmfile(void);
void work_cancel(void);

#endif
