#ifndef SEQ_H
#define SEQ_H

#include "dcp_sched/limits.h"
#include "dcp_sched/seq.h"
#include <stdint.h>

enum rc seq_module_init(void);
enum rc seq_submit(struct sched_seq *seq);
void seq_module_del(void);

#endif
