#ifndef MATCH_H
#define MATCH_H

struct imm_seq;
struct imm_step;

struct match
{
    struct imm_step const *step;
    struct imm_seq const *frag;
};

#endif
