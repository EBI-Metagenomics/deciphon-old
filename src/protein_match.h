#ifndef PROTEIN_MATCH_H
#define PROTEIN_MATCH_H

#include "imm/imm.h"
#include "match.h"
#include "rc.h"
#include <stdio.h>

struct protein_match
{
    struct match match;

    struct protein_profile const *profile;
};

int protein_match_write_cb(FILE *fp, void const *match);

#endif
