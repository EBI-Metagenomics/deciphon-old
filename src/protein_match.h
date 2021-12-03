#ifndef PROTEIN_MATCH_H
#define PROTEIN_MATCH_H

#include "imm/imm.h"
#include "rc.h"
#include <stdio.h>

#define PROTEIN_MATCH_FRAG_SIZE 6

struct protein_match
{
    unsigned frag_size;
    char const *frag;
    char state[IMM_STATE_NAME_SIZE];
    char codon[4];
    char amino[2];
};

static inline void protein_match_init(struct protein_match *match)
{
    match->frag_size = 0;
    match->frag = NULL;
    match->state[0] = 0;
    match->codon[0] = 0;
    match->amino[0] = 0;
}

static inline void protein_match_set_frag(struct protein_match *match, unsigned size,
                                      char const *frag)
{
    match->frag_size = size;
    match->frag = frag;
}

static inline char *protein_match_get_state_name(struct protein_match *match)
{
    return match->state;
}

static inline void protein_match_set_codon(struct protein_match *match,
                                       struct imm_codon codon)
{
    match->codon[0] = imm_codon_asym(&codon);
    match->codon[1] = imm_codon_bsym(&codon);
    match->codon[2] = imm_codon_csym(&codon);
}

static inline void protein_match_set_amino(struct protein_match *match, char amino)
{
    match->amino[0] = amino;
}

#endif
