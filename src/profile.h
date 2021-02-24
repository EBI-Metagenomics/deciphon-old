#ifndef PROFILE_H
#define PROFILE_H

#include <inttypes.h>

struct dcp_profile;
struct nmm_profile;

struct dcp_profile const* profile_create(uint32_t id, struct nmm_profile const* prof);
struct nmm_profile const* profile_nmm(struct dcp_profile const* prof);

#endif
