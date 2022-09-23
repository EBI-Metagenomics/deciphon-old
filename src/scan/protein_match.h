#ifndef SCAN_PROTEIN_MATCH_H
#define SCAN_PROTEIN_MATCH_H

#include "core/rc.h"
#include "imm/imm.h"
#include "scan/match.h"
#include <stdio.h>

struct protein_match
{
    struct match match;
};

enum rc protein_match_write(FILE *fp, void const *match);

#endif
