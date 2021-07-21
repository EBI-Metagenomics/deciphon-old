#ifndef PROFILE_H
#define PROFILE_H

#include "dcp/profile.h"

struct dcp_profile *profile_new(struct imm_abc const *abc,
                                struct dcp_metadata mt,
                                struct dcp_profile_vtable vtable);

void profile_del(struct dcp_profile const *prof);

#endif
