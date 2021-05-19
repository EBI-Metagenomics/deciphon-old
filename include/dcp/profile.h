#ifndef DCP_PROFILE_H
#define DCP_PROFILE_H

#include "dcp/export.h"
#include "dcp/metadata.h"
#include "imm/imm.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define DCP_PROFILE_NULL_IDX UINT32_MAX

typedef uint32_t dcp_profile_idx_t;

struct imm_dp;

struct dcp_profile
{
    dcp_profile_idx_t idx;
    struct dcp_metadata mt;
    struct
    {
        struct imm_dp *null;
        struct imm_dp *alt;
    } dp;
};

DCP_API void dcp_profile_init(struct dcp_profile *prof,
                              struct imm_abc const *abc);

DCP_API void dcp_profile_del(struct dcp_profile *prof);

#endif
