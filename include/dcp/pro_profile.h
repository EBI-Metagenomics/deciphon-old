#ifndef DCP_PRO_PROFILE_H
#define DCP_PRO_PROFILE_H

#include "dcp/export.h"
#include "dcp/pro_model.h"
#include "dcp/profile.h"
#include "imm/imm.h"
#include <assert.h>

struct dcp_pro_profile;

DCP_API struct dcp_pro_profile *
dcp_pro_profile_new(struct imm_amino const *amino,
                    struct imm_nuclt const *nuclt, enum dcp_entry_distr edist,
                    imm_float epsilon);

DCP_API void dcp_pro_profile_del(struct dcp_pro_profile const *p);

DCP_API void dcp_pro_profile_setup(struct dcp_pro_profile *p, unsigned seq_len,
                                   bool multihits, bool hmmer3_compat);

DCP_API int dcp_pro_profile_init(struct dcp_pro_profile *p,
                                 struct dcp_pro_model const *m);

DCP_API struct dcp_profile *dcp_pro_profile_super(struct dcp_pro_profile *pro);

DCP_API struct imm_dp const *
dcp_pro_profile_null_dp(struct dcp_pro_profile *pro);

DCP_API struct imm_dp const *
dcp_pro_profile_alt_dp(struct dcp_pro_profile *pro);

#endif
