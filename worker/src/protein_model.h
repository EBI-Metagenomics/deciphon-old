#ifndef PROTEIN_MODEL_H
#define PROTEIN_MODEL_H

#include "common/limits.h"
#include "entry_dist.h"
#include "imm/imm.h"
#include "nuclt_dist.h"
#include "protein_cfg.h"
#include "protein_node.h"
#include "protein_state.h"
#include "protein_trans.h"
#include "protein_xnode.h"
#include "protein_xtrans.h"
#include "common/rc.h"

struct protein_model
{
    struct imm_amino const *amino;
    struct imm_nuclt_code const *code;
    struct protein_cfg cfg;
    unsigned core_size;
    struct protein_xnode xnode;
    struct protein_xtrans xtrans;
    char consensus[PROTEIN_MODEL_CORE_SIZE_MAX + 1];

    struct
    {
        imm_float lprobs[IMM_AMINO_SIZE];
        struct nuclt_dist nucltd;
        struct imm_hmm hmm;
    } null;

    struct
    {
        unsigned node_idx;
        struct protein_node *nodes;
        imm_float *locc;
        unsigned trans_idx;
        struct protein_trans *trans;
        struct imm_hmm hmm;

        struct
        {
            struct nuclt_dist nucltd;
        } insert;
    } alt;
};

enum rc protein_model_add_node(struct protein_model *,
                               imm_float const lp[IMM_AMINO_SIZE],
                               char consensus);

enum rc protein_model_add_trans(struct protein_model *,
                                struct protein_trans trans);

void protein_model_del(struct protein_model const *);

void protein_model_init(struct protein_model *, struct imm_amino const *amino,
                        struct imm_nuclt_code const *code,
                        struct protein_cfg cfg,
                        imm_float const null_lprobs[IMM_AMINO_SIZE]);

enum rc protein_model_setup(struct protein_model *, unsigned core_size);

void protein_model_write_dot(struct protein_model const *, FILE *fp);

struct protein_model_summary
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

struct protein_model;

struct imm_amino const *protein_model_amino(struct protein_model const *m);
struct imm_nuclt const *protein_model_nuclt(struct protein_model const *m);
struct protein_model_summary
protein_model_summary(struct protein_model const *m);

#endif
