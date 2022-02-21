#ifndef DCP_MATCH_H
#define DCP_MATCH_H

struct imm_profile;
struct imm_seq;
struct imm_step;

struct match
{
    struct imm_step const *step;
    struct imm_seq const *frag;
    struct profile const *profile;
};

#endif
