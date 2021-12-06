#ifndef PROFILE_TYPES_H
#define PROFILE_TYPES_H

#include "entry_dist.h"
#include "imm/imm.h"
#include <stdint.h>
#include <stdio.h>

static_assert(sizeof(imm_state_id_t) == 2, "16 bits state_id.");
#define PROFILE_BITS_ID 16

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
