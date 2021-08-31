#ifndef DCP_PROFILE_TYPES_H
#define DCP_PROFILE_TYPES_H

#include "dcp/entry_dist.h"
#include "imm/imm.h"
#include <stdint.h>
#include <stdio.h>

typedef uint32_t dcp_nprofiles_t;
#define DCP_NPROFILES_MAX (1U << 20)

typedef uint32_t dcp_profile_idx_t;
#define DCP_PROFILE_NULL_IDX UINT32_MAX

static_assert(sizeof(imm_state_id_t) == 2, "16 bits state_id.");
#define DCP_PROFILE_BITS_ID 16

enum dcp_profile_typeid
{
    DCP_NULL_PROFILE = 0,
    DCP_STD_PROFILE = 1,
    DCP_PROTEIN_PROFILE = 2,
};

struct dcp_profile;

struct dcp_profile_vtable
{
    enum dcp_rc (*read)(struct dcp_profile *prof, FILE *restrict fd);
    enum dcp_rc (*write)(struct dcp_profile const *prof, FILE *restrict fd);
    void (*del)(struct dcp_profile *prof);
    enum dcp_profile_typeid typeid;
    void *derived;
};

#endif
