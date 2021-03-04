#ifndef PROFILE_H
#define PROFILE_H

#include <stdint.h>

struct dcp_profile;
struct nmm_profile;

struct dcp_profile* profile_create(uint32_t id, struct nmm_profile* prof);

#endif
