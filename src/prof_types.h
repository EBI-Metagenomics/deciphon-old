#ifndef PROF_TYPES_H
#define PROF_TYPES_H

#include "entry_dist.h"
#include "imm/imm.h"
#include <stdint.h>
#include <stdio.h>

typedef uint32_t dcp_nprofiles_t;
#define DCP_NPROFILES_MAX (1U << 20)

typedef uint32_t dcp_profile_idx_t;
#define DCP_PROFILE_NULL_IDX UINT32_MAX

static_assert(sizeof(imm_state_id_t) == 2, "16 bits state_id.");
#define DCP_PROFILE_BITS_ID 16

enum dcp_prof_typeid
{
    DCP_NULL_PROFILE,
    DCP_STD_PROFILE,
    DCP_PRO_PROFILE,
};

struct dcp_prof;

struct dcp_prof_vtable
{
    void (*del)(struct dcp_prof *prof);
    enum dcp_prof_typeid typeid;
};

#endif
