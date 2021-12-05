#ifndef PROFILE_TYPES_H
#define PROFILE_TYPES_H

#include "entry_dist.h"
#include "imm/imm.h"
#include <stdint.h>
#include <stdio.h>

typedef uint32_t dcp_nprofiles_t;
#define DCP_NPROFILES_MAX (1U << 20)

typedef uint32_t profile_idx_t;
#define DCP_PROFILE_NULL_IDX UINT32_MAX

static_assert(sizeof(imm_state_id_t) == 2, "16 bits state_id.");
#define DCP_PROFILE_BITS_ID 16

enum profile_typeid
{
    NULL_PROFILE,
    STANDARD_PROFILE,
    PROTEIN_PROFILE,
};

struct profile;

struct profile_vtable
{
    void (*del)(struct profile *prof);
    enum profile_typeid typeid;
};

#endif
