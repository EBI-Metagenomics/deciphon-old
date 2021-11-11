#ifndef PRO_MATCH_H
#define PRO_MATCH_H

#include "dcp/rc.h"
#include "imm/imm.h"
#include <stdio.h>

#define PRO_MATCH_FRAG_SIZE 6

struct pro_match
{
    unsigned frag_size;
    char const *frag;
    char state[IMM_STATE_NAME_SIZE];
    char codon[3];
    char amino;
};

#define PRO_MATCH_INIT() {0}

static inline void pro_match_set_frag(struct pro_match *match, unsigned size,
                                      char const *frag)
{
    match->frag_size = size;
    match->frag = frag;
}

static inline char *pro_match_get_state(struct pro_match *match)
{
    return match->state;
}

static inline void pro_match_set_codon(struct pro_match *match,
                                       struct imm_codon codon)
{
    match->codon[0] = imm_codon_asym(&codon);
    match->codon[1] = imm_codon_bsym(&codon);
    match->codon[2] = imm_codon_csym(&codon);
}

static inline void pro_match_set_amino(struct pro_match *match, char amino)
{
    match->amino = amino;
}

#endif
