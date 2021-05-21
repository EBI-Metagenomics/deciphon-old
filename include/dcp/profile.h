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

static inline struct dcp_profile dcp_profile(struct imm_abc const *abc)
{
    struct dcp_profile prof;
    prof.idx = DCP_PROFILE_NULL_IDX;
    prof.mt = dcp_metadata(NULL, NULL);
    prof.dp.null = imm_dp_new(abc);
    prof.dp.alt = imm_dp_new(abc);
    return prof;
}

DCP_API void dcp_profile_del(struct dcp_profile *prof);

#endif
