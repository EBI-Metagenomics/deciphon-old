#ifndef PROTEIN_MATCH_H
#define PROTEIN_MATCH_H

#include "imm/imm.h"
#include "match.h"
#include "common/rc.h"
#include <stdio.h>

struct protein_match
{
    struct match match;
};

enum rc protein_match_write_cb(FILE *fp, void const *match);

#endif
