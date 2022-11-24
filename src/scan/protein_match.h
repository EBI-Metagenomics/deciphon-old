#ifndef SCAN_PROTEIN_MATCH_H
#define SCAN_PROTEIN_MATCH_H

#include "imm/imm.h"
#include "rc.h"
#include "scan/match.h"
#include <stdbool.h>
#include <stdio.h>

struct protein_match
{
    struct match match;
    bool mute;
    struct imm_codon codon;
    char amino;
};

struct protein_profile;

void protein_match_init(struct protein_match *, struct protein_profile const *);
enum rc protein_match_setup(struct protein_match *pm,
                            struct imm_step const *step,
                            struct imm_seq const *frag);
enum rc protein_match_write(struct protein_match const *, FILE *);

#endif
