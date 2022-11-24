#ifndef SCAN_MATCH_H
#define SCAN_MATCH_H

struct imm_profile;
struct imm_seq;
struct imm_step;

struct match
{
    struct profile const *profile;
    struct imm_step const *step;
    struct imm_seq const *frag;
};

static inline void match_init(struct match *match,
                              struct profile const *profile)
{
    match->profile = profile;
    match->step = 0;
    match->frag = 0;
}

static inline void match_setup(struct match *match, struct imm_step const *step,
                               struct imm_seq const *frag)
{
    match->step = step;
    match->frag = frag;
}

#endif
