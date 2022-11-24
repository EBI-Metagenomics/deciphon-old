#ifndef SCAN_MATCH_H
#define SCAN_MATCH_H

#include "imm/imm.h"

struct imm_profile;
struct imm_seq;
struct imm_step;

struct match
{
    struct profile const *profile;
    struct imm_step step;
    struct imm_seq frag;
};

static inline void match_init(struct match *match,
                              struct profile const *profile)
{
    match->profile = profile;
}

static inline void match_setup(struct match *match, struct imm_step step,
                               struct imm_seq frag)
{
    match->step = step;
    match->frag = frag;
}

#endif
