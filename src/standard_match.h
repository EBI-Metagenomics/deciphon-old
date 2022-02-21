#ifndef DCP_STANDARD_MATCH_H
#define DCP_STANDARD_MATCH_H

#include "match.h"
#include <stdio.h>

struct standard_profile;

struct standard_match
{
    struct match match;
};

enum rc standard_match_write_cb(FILE *fp, void const *match);

#endif
