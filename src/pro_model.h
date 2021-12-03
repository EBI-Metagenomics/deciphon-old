#ifndef PRO_MODEL_H
#define PRO_MODEL_H

#include "entry_dist.h"
#include "imm/imm.h"
#include "nuclt_dist.h"
#include "pro_cfg.h"
#include "pro_id.h"
#include "pro_node.h"
#include "pro_state.h"
#include "pro_trans.h"
#include "pro_xnode.h"
#include "pro_xtrans.h"
#include "rc.h"

#define DCP_PRO_MODEL_CORE_SIZE_MAX 2048

struct pro_model
{
    struct imm_amino const *amino;
    struct imm_nuclt_code const *code;
    struct dcp_pro_cfg cfg;
    unsigned core_size;
    struct dcp_pro_xnode xnode;
    struct dcp_pro_xtrans xtrans;
    char consensus[DCP_PRO_MODEL_CORE_SIZE_MAX + 1];

    struct
    {
        imm_float lprobs[IMM_AMINO_SIZE];
        struct dcp_nuclt_dist nucltd;
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
            struct dcp_nuclt_dist nucltd;
        } insert;
    } alt;
};

enum rc dcp_pro_model_add_node(struct pro_model *,
                               imm_float const lp[IMM_AMINO_SIZE],
                               char consensus);

enum rc dcp_pro_model_add_trans(struct pro_model *, struct dcp_pro_trans trans);

void dcp_pro_model_del(struct pro_model const *);

void dcp_pro_model_init(struct pro_model *, struct imm_amino const *amino,
                        struct imm_nuclt_code const *code,
                        struct dcp_pro_cfg cfg,
                        imm_float const null_lprobs[IMM_AMINO_SIZE]);

enum rc dcp_pro_model_setup(struct pro_model *, unsigned core_size);

void dcp_pro_model_write_dot(struct pro_model const *, FILE *restrict fp);

struct pro_model_summary
{
    struct
    {
        struct imm_hmm const *hmm;
        struct imm_frame_state const *R;
    } null;

    struct
    {
        struct imm_hmm const *hmm;
        struct imm_mute_state const *S;
        struct imm_frame_state const *N;
        struct imm_mute_state const *B;
        struct imm_mute_state const *E;
        struct imm_frame_state const *J;
        struct imm_frame_state const *C;
        struct imm_mute_state const *T;
    } alt;
};

struct pro_model;

struct imm_amino const *pro_model_amino(struct pro_model const *m);
struct imm_nuclt const *pro_model_nuclt(struct pro_model const *m);
struct pro_model_summary pro_model_summary(struct pro_model const *m);

#endif
