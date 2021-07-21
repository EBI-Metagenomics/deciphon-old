#ifndef DCP_PROFILE_H
#define DCP_PROFILE_H

#include "dcp/export.h"
#include "dcp/metadata.h"
#include "dcp/profile_types.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct imm_abc;

struct dcp_profile
{
    unsigned idx;
    struct imm_abc const *abc;
    struct dcp_metadata mt;
    struct dcp_profile_vtable vtable;
};

static inline void dcp_profile_del(struct dcp_profile const *prof)
{
    prof->vtable.del(prof);
}

#endif
