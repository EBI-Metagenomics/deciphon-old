#ifndef PROD_H
#define PROD_H

#include "common/limits.h"
#include "sched/prod.h"
#include "sched/sched.h"
#include <stdint.h>
#include <stdio.h>

struct protein_match;

enum rc prod_begin_submission(unsigned num_threads);
enum rc prod_end_submission(void);
enum rc prod_get(struct sched_prod *prod);

#endif
