#ifndef MODEL_PROT_MODEL_H
#define MODEL_PROT_MODEL_H

#include "deciphon_limits.h"
#include "imm/imm.h"
#include "model/entry_dist.h"
#include "model/nuclt_dist.h"
#include "model/prot_cfg.h"
#include "model/prot_node.h"
#include "model/prot_state.h"
#include "model/prot_trans.h"
#include "model/prot_xnode.h"
#include "model/prot_xtrans.h"
#include "rc.h"

struct prot_model
{
    struct imm_amino const *amino;
    struct imm_nuclt_code const *code;
    struct prot_cfg cfg;
    unsigned core_size;
    struct prot_xnode xnode;
    struct prot_xtrans xtrans;
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
        struct prot_node *nodes;
        imm_float *locc;
        unsigned trans_idx;
        struct prot_trans *trans;
        struct imm_hmm hmm;

        struct
        {
            struct nuclt_dist nucltd;
        } insert;
    } alt;
};

enum rc prot_model_add_node(struct prot_model *,
                            imm_float const lp[IMM_AMINO_SIZE], char consensus);

enum rc prot_model_add_trans(struct prot_model *, struct prot_trans trans);

void prot_model_del(struct prot_model const *);

void prot_model_init(struct prot_model *, struct imm_amino const *amino,
                     struct imm_nuclt_code const *code, struct prot_cfg cfg,
                     imm_float const null_lprobs[IMM_AMINO_SIZE]);

enum rc prot_model_setup(struct prot_model *, unsigned core_size);

void prot_model_write_dot(struct prot_model const *, FILE *fp);

struct prot_model_summary
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

struct prot_model;

struct imm_amino const *prot_model_amino(struct prot_model const *m);
struct imm_nuclt const *prot_model_nuclt(struct prot_model const *m);
struct prot_model_summary prot_model_summary(struct prot_model const *m);

#endif
