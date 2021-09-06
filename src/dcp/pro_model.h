#ifndef DCP_PRO_MODEL_H
#define DCP_PRO_MODEL_H

#include "dcp/entry_dist.h"
#include "dcp/export.h"
#include "dcp/pro_cfg.h"
#include "dcp/pro_id.h"
#include "dcp/pro_node.h"
#include "dcp/pro_trans.h"
#include "dcp/pro_xnode.h"
#include "dcp/pro_xtrans.h"
#include "dcp/rc.h"

struct dcp_pro_model
{
    struct imm_amino const *amino;
    struct imm_nuclt const *nuclt;
    struct dcp_pro_cfg cfg;
    unsigned core_size;
    struct dcp_pro_xnode xnode;
    struct dcp_pro_xtrans xtrans;

    struct
    {
        imm_float lprobs[IMM_AMINO_SIZE];
        struct imm_nuclt_lprob nucltp;
        struct imm_codon_marg codonm;
        struct imm_hmm hmm;
    } null;

    struct
    {
        unsigned node_idx;
        struct dcp_pro_node *nodes;
        imm_float *locc;
        unsigned trans_idx;
        struct dcp_pro_trans *trans;
        struct imm_hmm hmm;

        struct
        {
            struct imm_nuclt_lprob nucltp;
            struct imm_codon_marg codonm;
        } insert;
    } alt;
};

DCP_API enum dcp_rc dcp_pro_model_add_node(struct dcp_pro_model *,
                                           imm_float const lp[IMM_AMINO_SIZE]);

DCP_API enum dcp_rc dcp_pro_model_add_trans(struct dcp_pro_model *,
                                            struct dcp_pro_trans trans);

DCP_API void dcp_pro_model_del(struct dcp_pro_model const *);

DCP_API void dcp_pro_model_init(struct dcp_pro_model *,
                                struct imm_amino const *amino,
                                struct imm_nuclt const *nuclt,
                                struct dcp_pro_cfg cfg,
                                imm_float const null_lprobs[IMM_AMINO_SIZE]);

DCP_API enum dcp_rc dcp_pro_model_setup(struct dcp_pro_model *,
                                        unsigned core_size);

DCP_API void dcp_pro_model_write_dot(struct dcp_pro_model const *,
                                     FILE *restrict fp);

#endif
