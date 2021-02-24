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

void dcp_profile_destroy(struct dcp_profile const* prof, bool deep)
{
    nmm_profile_destroy(prof->nmm_profile, deep);
    free_c(prof);
}

uint32_t dcp_profile_id(struct dcp_profile const* prof) { return prof->id; }

struct dcp_profile const* profile_create(uint32_t id, struct nmm_profile const* prof)
{
    struct dcp_profile* p = malloc(sizeof(*p));
    p->id = id;
    p->nmm_profile = prof;
    return p;
}

struct nmm_profile const* profile_nmm(struct dcp_profile const* prof) { return prof->nmm_profile; }
