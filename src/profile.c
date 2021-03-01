#include "profile.h"
#include "deciphon/deciphon.h"
#include "free.h"
#include "nmm/nmm.h"
#include <stdlib.h>

struct dcp_profile
{
    uint32_t                  id;
    struct nmm_profile const* nmm_profile;
};

struct imm_abc const* dcp_profile_abc(struct dcp_profile const* prof) { return nmm_profile_abc(prof->nmm_profile); }

void dcp_profile_destroy(struct dcp_profile const* prof, bool deep)
{
    nmm_profile_destroy(prof->nmm_profile, deep);
    free_c(prof);
}

struct imm_model* dcp_profile_get_model(struct dcp_profile const* prof, uint8_t i)
{
    return nmm_profile_get_model(prof->nmm_profile, i);
}

uint32_t dcp_profile_id(struct dcp_profile const* prof) { return prof->id; }

uint8_t dcp_profile_nmodels(struct dcp_profile const* prof) { return nmm_profile_nmodels(prof->nmm_profile); }

struct dcp_profile const* profile_create(uint32_t id, struct nmm_profile const* prof)
{
    struct dcp_profile* p = malloc(sizeof(*p));
    p->id = id;
    p->nmm_profile = prof;
    return p;
}

struct nmm_profile const* profile_nmm(struct dcp_profile const* prof) { return prof->nmm_profile; }
