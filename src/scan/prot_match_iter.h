#ifndef PROT_MATCH_ITER_H
#define PROT_MATCH_ITER_H

#include "scan/prot_match.h"
#include "scan/prot_result.h"

struct prot_match_iter
{
    struct prot_result const *result;
    struct prot_match match;
    unsigned idx;
    unsigned offset;
};

void prot_match_iter(struct prot_match_iter *, struct prot_result const *);

struct prot_match const *prot_match_iter_next(struct prot_match_iter *);

#endif
