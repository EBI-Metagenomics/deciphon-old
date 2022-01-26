#ifndef STANDARD_PROFILE_H
#define STANDARD_PROFILE_H

#include "common/rc.h"
#include "profile.h"
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

void standard_profile_init(struct standard_profile *, struct imm_code const *);
enum rc standard_profile_read(struct standard_profile *prof, FILE *fp);
enum rc standard_profile_write(struct standard_profile const *prof,
                               struct cmp_ctx_s *cmp);

#endif
