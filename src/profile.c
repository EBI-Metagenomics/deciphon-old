#include "profile.h"
#include "dcp/profile.h"
#include "special_trans.h"
#include "util.h"

void dcp_profile_del(struct dcp_profile *prof)
{
    imm_dp_del(prof->dp.null);
    imm_dp_del(prof->dp.alt);
}

#if 0
void profile_setup(struct imm_hmm *hmm, struct imm_dp *dp, bool multiple_hits,
                   uint32_t target_length, bool hmmer3_compat)
{
    struct special_trans trans =
        special_trans_get(multiple_hits, target_length, hmmer3_compat);
    special_trans_set(trans, hmm, dp);
}
#endif
