#ifndef PROFILE_H
#define PROFILE_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

struct dcp_metadata;
struct dcp_profile;
struct imm_dp;
struct imm_hmm;
struct nmm_profile;

#if 0
struct dcp_profile* profile_create(uint32_t id, struct nmm_profile* prof, struct dcp_metadata const* mt);
void                profile_setup(struct imm_hmm* hmm, struct imm_dp* dp, bool multiple_hits, uint32_t target_length,
                                  bool hmmer3_compat);
#endif
int profile_read(struct dcp_profile *prof, FILE *fd);

#endif
