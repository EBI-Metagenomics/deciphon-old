#ifndef PRO_MODEL_H
#define PRO_MODEL_H

#include "imm/imm.h"

struct pro_model_special_trans
{
    imm_float NN;
    imm_float CC;
    imm_float JJ;
    imm_float NB;
    imm_float CT;
    imm_float JB;
    imm_float RR;
    imm_float EJ;
    imm_float EC;
};

static inline struct pro_model_special_trans pro_model_special_trans_init(void)
{
    return (struct pro_model_special_trans){.NN = IMM_LPROB_ONE,
                                            .NB = IMM_LPROB_ONE,
                                            .EC = IMM_LPROB_ONE,
                                            .CC = IMM_LPROB_ONE,
                                            .CT = IMM_LPROB_ONE,
                                            .EJ = IMM_LPROB_ONE,
                                            .JJ = IMM_LPROB_ONE,
                                            .JB = IMM_LPROB_ONE,
                                            .RR = IMM_LPROB_ONE};
}

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

struct dcp_pro_model;

struct imm_amino const *pro_model_amino(struct dcp_pro_model const *m);
struct imm_nuclt const *pro_model_nuclt(struct dcp_pro_model const *m);
struct pro_model_summary pro_model_summary(struct dcp_pro_model const *m);
void pro_model_state_name(unsigned id, char name[IMM_STATE_NAME_SIZE]);

#endif
