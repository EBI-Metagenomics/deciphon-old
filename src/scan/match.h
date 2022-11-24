#ifndef SCAN_MATCH_H
#define SCAN_MATCH_H

struct imm_profile;
struct imm_seq;
struct imm_step;

struct match
{
    struct imm_step const *step;
    struct imm_seq const *frag;
    struct profile const *profile;
};

static inline void match_init(struct match *match, struct imm_step const *step,
                              struct imm_seq const *frag,
                              struct profile const *profile)
{
    match->step = step;
    match->frag = frag;
    match->profile = profile;
}

static inline void match_copy(struct match *dst, struct match const *src)
{
    dst->step = src->step;
    dst->frag = src->frag;
    dst->profile = src->profile;
}

static inline void match_setup(struct match *match,
                               struct profile const *profile)
{
    match->step = 0;
    match->frag = 0;
    match->profile = profile;
}

#endif
