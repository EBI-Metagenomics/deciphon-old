#ifndef DCP_PRO_PROFILE_H
#define DCP_PRO_PROFILE_H

#include "dcp/export.h"
#include "dcp/meta.h"
#include "dcp/pro_model.h"
#include "dcp/profile.h"
#include "imm/imm.h"
#include <assert.h>

struct dcp_pro_profile
{
    struct dcp_profile super;
    struct dcp_pro_cfg cfg;

    struct
    {
        struct imm_dp dp;
        unsigned R;
    } null;

    struct
    {
        struct imm_dp dp;
        unsigned S;
        unsigned N;
        unsigned B;
        unsigned E;
        unsigned J;
        unsigned C;
        unsigned T;
    } alt;
};

DCP_API void dcp_pro_profile_init(struct dcp_pro_profile *p,
                                  struct dcp_pro_cfg cfg);

DCP_API void dcp_pro_profile_setup(struct dcp_pro_profile *p, unsigned seq_len,
                                   bool multihits, bool hmmer3_compat);

DCP_API int dcp_pro_profile_absorb(struct dcp_pro_profile *p,
                                   struct dcp_pro_model const *m);

DCP_API struct dcp_profile *dcp_pro_profile_super(struct dcp_pro_profile *pro);

DCP_API void dcp_pro_profile_state_name(unsigned id, char name[8]);

DCP_API void dcp_pro_profile_sample(struct dcp_pro_profile *p, unsigned seed,
                                    unsigned core_size,
                                    enum dcp_entry_dist edist,
                                    imm_float epsilon);

static inline void dcp_pro_profile_del(struct dcp_pro_profile *pro)
{
    if (pro)
        dcp_profile_del(&pro->super);
}

#endif
