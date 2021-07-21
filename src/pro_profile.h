#ifndef PRO_PROFILE_H
#define PRO_PROFILE_H

#include "dcp/entry_distr.h"
#include "imm/imm.h"

struct dcp_profile;
struct imm_dp;

struct dcp_pro_profile
{
    struct dcp_profile *super;
    struct imm_amino const *amino;
    struct imm_nuclt const *nuclt;
    enum dcp_entry_distr edist;
    imm_float epsilon;

    struct
    {
        struct imm_dp *dp;
        unsigned R;
    } null;

    struct
    {
        struct imm_dp *dp;
        unsigned S;
        unsigned N;
        unsigned B;
        unsigned E;
        unsigned J;
        unsigned C;
        unsigned T;
    } alt;
};

#endif
