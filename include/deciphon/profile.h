#ifndef DECIPHON_PROFILE_H
#define DECIPHON_PROFILE_H

#include "deciphon/export.h"
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>

#define DCP_PROFILE_INVALID_PROFID UINT32_MAX

struct dcp_metadata;
struct dcp_profile;
struct imm_dp;
struct imm_hmm;
struct imm_model;
struct nmm_profile;

DCP_API struct imm_abc const*      dcp_profile_abc(struct dcp_profile const* prof);
DCP_API void                       dcp_profile_append_model(struct dcp_profile* prof, struct imm_model* model);
DCP_API struct dcp_profile*        dcp_profile_create(struct imm_abc const* abc, struct dcp_metadata const* mt);
DCP_API void                       dcp_profile_destroy(struct dcp_profile const* prof, bool deep);
DCP_API void                       dcp_profile_free(struct dcp_profile const* prof);
DCP_API struct imm_model*          dcp_profile_model(struct dcp_profile const* prof, uint8_t i);
DCP_API uint32_t                   dcp_profile_id(struct dcp_profile const* prof);
DCP_API struct dcp_metadata const* dcp_profile_metadata(struct dcp_profile const* prof);
DCP_API struct nmm_profile const*  dcp_profile_nmm_profile(struct dcp_profile const* prof);
DCP_API uint8_t                    dcp_profile_nmodels(struct dcp_profile const* prof);

/* TODO: Temporary? */
DCP_API void dcp_profile_setup(struct imm_hmm* hmm, struct imm_dp* dp, bool multiple_hits, uint32_t target_length,
                               bool hmmer3_compat);

#endif
