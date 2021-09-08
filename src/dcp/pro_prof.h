#ifndef DCP_PRO_PROF_H
#define DCP_PRO_PROF_H

#include "dcp/export.h"
#include "dcp/meta.h"
#include "dcp/pro_cfg.h"
#include "dcp/pro_model.h"
#include "dcp/prof.h"
#include "dcp/rc.h"
#include "imm/imm.h"

struct dcp_pro_prof
{
    struct dcp_prof super;

    struct imm_amino const *amino;
    struct imm_nuclt const *nuclt;
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

DCP_API void dcp_pro_prof_init(struct dcp_pro_prof *prof,
                               struct imm_amino const *amino,
                               struct imm_nuclt const *nuclt,
                               struct dcp_pro_cfg cfg);

DCP_API enum dcp_rc dcp_pro_prof_setup(struct dcp_pro_prof *prof,
                                       unsigned seq_size, bool multi_hits,
                                       bool hmmer3_compat);

DCP_API enum dcp_rc dcp_pro_prof_absorb(struct dcp_pro_prof *prof,
                                        struct dcp_pro_model const *model);

DCP_API struct dcp_prof *dcp_pro_prof_super(struct dcp_pro_prof *pro);

DCP_API void dcp_pro_prof_state_name(unsigned id, char[IMM_STATE_NAME_SIZE]);

DCP_API enum dcp_rc dcp_pro_prof_sample(struct dcp_pro_prof *prof,
                                        unsigned seed, unsigned core_size);

static inline void dcp_pro_prof_del(struct dcp_pro_prof *prof)
{
    if (prof) dcp_prof_del(&prof->super);
}

DCP_API void dcp_pro_prof_write_dot(struct dcp_pro_prof const *prof,
                                    FILE *restrict fp);

#endif
