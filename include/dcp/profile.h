#ifndef DCP_PROFILE_H
#define DCP_PROFILE_H

#include "dcp/export.h"
#include "dcp/metadata.h"
#include "dcp/pp.h"
#include "imm/imm.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define DCP_PROFILE_NULL_IDX UINT32_MAX

typedef uint32_t dcp_profile_idx_t;

struct dcp_profile
{
    struct imm_abc const *abc;
    dcp_profile_idx_t idx;
    struct dcp_metadata mt;
    struct
    {
        struct imm_dp *null;
        struct imm_dp *alt;
    } dp;
    struct
    {
        imm_float epsilon;
        struct dcp_pp_special_states states;
    } protein;
};

#define DCP_PROFILE_INIT(abc)                                                  \
    ({                                                                         \
        struct dcp_profile __prof = {0};                                       \
        dcp_profile_init(abc, &__prof);                                        \
        __prof;                                                                \
    })

DCP_API void dcp_profile_init(struct imm_abc const *abc,
                              struct dcp_profile *prof);

DCP_API void dcp_profile_deinit(struct dcp_profile *prof);

#endif
