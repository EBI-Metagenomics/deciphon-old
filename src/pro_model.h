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

struct pro_model_summary
{
    struct
    {
        struct imm_hmm *hmm;
        struct imm_frame_state *R;
    } null;

    struct
    {
        struct imm_hmm *hmm;
        struct imm_mute_state *S;
        struct imm_frame_state *N;
        struct imm_mute_state *B;
        struct imm_mute_state *E;
        struct imm_frame_state *J;
        struct imm_frame_state *C;
        struct imm_mute_state *T;
    } alt;
};

struct dcp_pro_model;

struct pro_model_summary pro_model_summary(struct dcp_pro_model const *m);

struct imm_amino const *pro_model_amino(struct dcp_pro_model const *m);

struct imm_nuclt const *pro_model_nuclt(struct dcp_pro_model const *m);

#endif
