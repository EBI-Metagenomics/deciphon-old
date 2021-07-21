#ifndef DCP_PROFILE_TYPES_H
#define DCP_PROFILE_TYPES_H

#include "dcp/entry_distr.h"
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
    DCP_NORMAL_PROFILE = 1,
    DCP_PROTEIN_PROFILE = 2,
};

struct dcp_profile;

struct dcp_profile_vtable
{
    int (*read)(struct dcp_profile *prof, FILE *restrict fd);
    int (*write)(struct dcp_profile const *prof, FILE *restrict fd);
    void (*del)(struct dcp_profile const *prof);
    enum dcp_profile_typeid typeid;
    void *derived;
};

struct dcp_profile_cfg
{
};

struct dcp_pro_profile_cfg
{
    struct dcp_profile_cfg super;
    imm_float epsilon;
    enum dcp_entry_distr entry_distr;
    struct imm_nuclt *nuclt;
    struct imm_amino *amino;
};

struct dcp_normal_profile_cfg
{
    struct dcp_profile_cfg super;
};

#endif
