#ifndef PROFILE_H
#define PROFILE_H

#include <stdint.h>
#include <stdio.h>

struct dcp_metadata;
struct dcp_profile;
struct nmm_profile;

struct dcp_profile* profile_create(uint32_t id, struct nmm_profile* prof, struct dcp_metadata const* mt);

#endif
