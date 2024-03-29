#ifndef PROTEIN_MATCH_H
#define PROTEIN_MATCH_H

#include "deciphon/core/rc.h"
#include "imm/imm.h"
#include "match.h"
#include <stdio.h>

struct protein_match
{
    struct match match;
};

enum rc protein_match_write_func(FILE *fp, void const *match);

#endif
