#ifndef SCAN_PROT_MATCH_H
#define SCAN_PROT_MATCH_H

#include "imm/imm.h"
#include "rc.h"
#include "scan/match.h"
#include <stdbool.h>
#include <stdio.h>

struct prot_match
{
    struct match match;
    bool mute;
    struct imm_codon codon;
    char amino;
};

struct prot_profile;

void prot_match_init(struct prot_match *, struct prot_profile const *);
enum rc prot_match_setup(struct prot_match *pm, struct imm_step step,
                         struct imm_seq frag);
enum rc prot_match_write(struct prot_match const *, FILE *);

#endif
