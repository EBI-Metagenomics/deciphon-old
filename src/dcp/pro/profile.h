#ifndef DCP_PRO_PROFILE_H
#define DCP_PRO_PROFILE_H

#include "dcp/export.h"
#include "dcp/meta.h"
#include "dcp/pro/model.h"
#include "dcp/profile.h"
#include "dcp/rc.h"
#include "imm/imm.h"

struct dcp_pro_prof
{
    struct dcp_prof super;
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

DCP_API void dcp_pro_prof_init(struct dcp_pro_prof *p, struct dcp_pro_cfg cfg);

DCP_API void dcp_pro_prof_setup(struct dcp_pro_prof *p, unsigned seq_len,
                                bool multi_hits, bool hmmer3_compat);

DCP_API enum dcp_rc dcp_pro_prof_absorb(struct dcp_pro_prof *p,
                                        struct dcp_pro_model const *m);

DCP_API struct dcp_prof *dcp_pro_prof_super(struct dcp_pro_prof *pro);

DCP_API void dcp_pro_prof_state_name(unsigned id,
                                     char name[IMM_STATE_NAME_SIZE]);

DCP_API void dcp_pro_prof_sample(struct dcp_pro_prof *p, unsigned seed,
                                 unsigned core_size, enum dcp_entry_dist edist,
                                 imm_float epsilon);

static inline void dcp_pro_prof_del(struct dcp_pro_prof *pro)
{
    if (pro) dcp_prof_del(&pro->super);
}

DCP_API void dcp_pro_profile_write_dot(struct dcp_pro_prof const *p,
                                       FILE *restrict fp);

#endif
