#ifndef MATCH_H
#define MATCH_H

struct imm_profile;
struct imm_seq;
struct imm_step;

struct match
{
    struct imm_step const *step;
    struct imm_seq const *frag;
    struct profile const *profile;
};

static inline void match_setup(struct match *match,
                               struct profile const *profile)
{
    match->step = 0;
    match->frag = 0;
    match->profile = profile;
}

#endif
