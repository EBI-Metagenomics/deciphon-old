#ifndef WORK_H
#define WORK_H

#include "rc.h"

void work_init(void);
enum rc work_next(void);
enum rc work_run(unsigned num_threads);

#endif
