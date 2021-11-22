#ifndef PROF_H
#define PROF_H

#include "dcp/prof.h"

static void profile_init(struct dcp_prof *prof, struct imm_code const *code,
                         struct dcp_meta mt, struct dcp_prof_vtable vtable)
{
    prof->vtable = vtable;
    prof->idx = DCP_PROFILE_NULL_IDX;
    prof->code = code;
    prof->mt = mt;
}

#endif
