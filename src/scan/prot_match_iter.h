#ifndef PROT_MATCH_ITER_H
#define PROT_MATCH_ITER_H

#include "scan/prot_match.h"

struct prot_match_iter
{
    struct imm_seq const *seq;
    struct imm_path const *path;
    struct prot_match match;
    unsigned idx;
    unsigned offset;
};

void prot_match_iter(struct prot_match_iter *, struct prot_profile const *prof,
                     struct imm_seq const *seq, struct imm_path const *path);

struct prot_match const *prot_match_iter_next(struct prot_match_iter *);

#endif
