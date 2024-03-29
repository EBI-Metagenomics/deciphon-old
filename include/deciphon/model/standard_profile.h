#ifndef DECIPHON_MODEL_STANDARD_PROFILE_H
#define DECIPHON_MODEL_STANDARD_PROFILE_H

#include "deciphon/core/rc.h"
#include "deciphon/model/profile.h"
#include <stdio.h>

struct standard_profile
{
    struct profile super;

    struct
    {
        struct imm_dp null;
        struct imm_dp alt;
    } dp;
};

void standard_profile_init(struct standard_profile *, char const *accession,
                           struct imm_code const *);
enum rc standard_profile_unpack(struct standard_profile *prof, FILE *fp);
enum rc standard_profile_pack(struct standard_profile const *prof,
                              struct lip_file *);

#endif
