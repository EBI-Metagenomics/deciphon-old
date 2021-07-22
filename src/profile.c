#include "profile.h"
#include "support.h"

struct dcp_profile *profile_new(struct imm_abc const *abc, struct dcp_meta mt,
                                struct dcp_profile_vtable vtable)
{
    struct dcp_profile *prof = xmalloc(sizeof(*prof));
    prof->idx = DCP_PROFILE_NULL_IDX;
    prof->abc = abc;
    prof->mt = mt;
    prof->vtable = vtable;
    return prof;
}

void profile_del(struct dcp_profile const *prof) { free((void *)prof); }

#if 0
void profile_setup(struct imm_hmm *hmm, struct imm_dp *dp, bool multiple_hits,
                   uint32_t target_length, bool hmmer3_compat)
{
    struct special_trans trans =
        special_trans_get(multiple_hits, target_length, hmmer3_compat);
    special_trans_set(trans, hmm, dp);
}
#endif
