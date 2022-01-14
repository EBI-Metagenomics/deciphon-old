#ifndef PROD_H
#define PROD_H

#include "sched/sched.h"
#include "sched/prod.h"
#include "imm/imm.h"
#include "common/logger.h"

struct match;
struct profile;

enum rc prod_write(struct sched_prod const *prod, struct imm_seq const *seq,
                   struct imm_path const *path, unsigned partition,
                   sched_prod_write_match_cb write_match_cb,
                   struct match *match);

#endif
