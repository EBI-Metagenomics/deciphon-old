#ifndef DECIPHON_PROFILE_H
#define DECIPHON_PROFILE_H

#include "deciphon/export.h"
#include <inttypes.h>
#include <stdbool.h>

struct dcp_profile;

DCP_API void     dcp_profile_destroy(struct dcp_profile const* prof, bool deep);
DCP_API uint32_t dcp_profile_id(struct dcp_profile const* prof);

#endif
