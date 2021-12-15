#ifndef PROD_H
#define PROD_H

#include "dcp_sched/sched.h"
#include "imm/imm.h"
#include "logger.h"

struct match;
struct profile;

enum rc prod_write(struct sched_prod const *prod, struct profile const *prof,
                   struct imm_seq const *seq, struct imm_path const *path,
                   unsigned partition, sched_prod_write_match_cb write_match_cb,
                   struct match *match);

#endif
