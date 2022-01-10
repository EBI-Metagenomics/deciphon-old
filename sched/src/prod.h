#ifndef PROD_H
#define PROD_H

#include "dcp_sched/limits.h"
#include "dcp_sched/prod.h"
#include "dcp_sched/sched.h"
#include <stdint.h>
#include <stdio.h>

struct protein_match;

enum rc prod_module_init(void);

enum rc prod_begin_submission(unsigned num_threads);
enum rc prod_end_submission(void);

void prod_module_del(void);

#endif
