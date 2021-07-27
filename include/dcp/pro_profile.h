#ifndef DCP_PRO_PROFILE_H
#define DCP_PRO_PROFILE_H

#include "dcp/export.h"
#include "dcp/meta.h"
#include "dcp/pro_model.h"
#include "dcp/profile.h"
#include "imm/imm.h"
#include <assert.h>

struct dcp_pro_profile;

DCP_API struct dcp_pro_profile *dcp_pro_profile_new(struct dcp_pro_cfg cfg,
                                                    struct dcp_meta mt);

DCP_API void dcp_pro_profile_setup(struct dcp_pro_profile *p, unsigned seq_len,
                                   bool multihits, bool hmmer3_compat);

DCP_API int dcp_pro_profile_init(struct dcp_pro_profile *p,
                                 struct dcp_pro_model const *m);

DCP_API struct dcp_profile *dcp_pro_profile_super(struct dcp_pro_profile *pro);

DCP_API struct imm_dp const *
dcp_pro_profile_null_dp(struct dcp_pro_profile *pro);

DCP_API struct imm_dp const *
dcp_pro_profile_alt_dp(struct dcp_pro_profile *pro);

DCP_API struct dcp_pro_cfg
dcp_pro_profile_cfg(struct dcp_pro_profile const *pro);

DCP_API void dcp_pro_profile_state_name(unsigned id, char name[8]);

static inline void dcp_pro_profile_del(struct dcp_pro_profile const *pro)
{
    if (pro)
        dcp_profile_del(dcp_pro_profile_super((struct dcp_pro_profile *)pro));
}

#endif
