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

enum rc protein_match_init(struct protein_match *,
                           struct protein_profile const *,
                           struct match const *);
enum rc protein_match_write(FILE *fp, struct match const *);

#endif
