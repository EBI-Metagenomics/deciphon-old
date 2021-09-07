#include "dcp/pro_prof.h"
#include "dcp/generics.h"
#include "dcp/prof.h"
#include "dcp/rc.h"
#include "error.h"
#include "imm/imm.h"
#include "meta.h"
#include "pro_model.h"
#include "pro_prof.h"
#include "prof.h"
#include "third-party/xrandom.h"
#include <assert.h>

static void del(struct dcp_prof *prof);

void dcp_pro_prof_init(struct dcp_pro_prof *p, struct imm_amino const *amino,
                       struct imm_nuclt const *nuclt, struct dcp_pro_cfg cfg)
{
    struct dcp_prof_vtable vtable = {del, DCP_PROTEIN_PROFILE};
    profile_init(&p->super, imm_super(nuclt), meta_unset, vtable);
    p->nuclt = nuclt;
    p->amino = amino;
    p->cfg = cfg;
    imm_dp_init(&p->null.dp, imm_super(nuclt));
    imm_dp_init(&p->alt.dp, imm_super(nuclt));
}

void dcp_pro_prof_setup(struct dcp_pro_prof *p, unsigned seq_len,
                        bool multi_hits, bool hmmer3_compat)
{
    assert(seq_len > 0);
    imm_float L = (imm_float)seq_len;

    imm_float q = 0.0;
    imm_float log_q = IMM_LPROB_ZERO;

    if (multi_hits)
    {
        q = 0.5;
        log_q = imm_log(0.5);
    }

    imm_float lp = imm_log(L) - imm_log(L + 2 + q / (1 - q));
    imm_float l1p = imm_log(2 + q / (1 - q)) - imm_log(L + 2 + q / (1 - q));
    imm_float lr = imm_log(L) - imm_log(L + 1);

    struct dcp_pro_xtrans t;

    t.NN = t.CC = t.JJ = lp;
    t.NB = t.CT = t.JB = l1p;
    t.RR = lr;
    t.EJ = log_q;
    t.EC = imm_log(1 - q);

    if (hmmer3_compat)
    {
        t.NN = t.CC = t.JJ = imm_log(1);
    }

    struct imm_dp *dp = &p->null.dp;
    unsigned R = p->null.R;
    imm_dp_change_trans(dp, imm_dp_trans_idx(dp, R, R), t.RR);

    dp = &p->alt.dp;
    unsigned S = p->alt.S;
    unsigned N = p->alt.N;
    unsigned B = p->alt.B;
    unsigned E = p->alt.E;
    unsigned J = p->alt.J;
    unsigned C = p->alt.C;
    unsigned T = p->alt.T;

    imm_dp_change_trans(dp, imm_dp_trans_idx(dp, S, B), t.NB);
    imm_dp_change_trans(dp, imm_dp_trans_idx(dp, S, N), t.NN);
    imm_dp_change_trans(dp, imm_dp_trans_idx(dp, N, N), t.NN);
    imm_dp_change_trans(dp, imm_dp_trans_idx(dp, N, B), t.NB);

    imm_dp_change_trans(dp, imm_dp_trans_idx(dp, E, T), t.EC + t.CT);
    imm_dp_change_trans(dp, imm_dp_trans_idx(dp, E, C), t.EC + t.CC);
    imm_dp_change_trans(dp, imm_dp_trans_idx(dp, C, C), t.CC);
    imm_dp_change_trans(dp, imm_dp_trans_idx(dp, C, T), t.CT);

    imm_dp_change_trans(dp, imm_dp_trans_idx(dp, E, B), t.EC + t.CT);
    imm_dp_change_trans(dp, imm_dp_trans_idx(dp, E, J), t.EC + t.CC);
    imm_dp_change_trans(dp, imm_dp_trans_idx(dp, J, J), t.CC);
    imm_dp_change_trans(dp, imm_dp_trans_idx(dp, J, B), t.CT);
}

enum dcp_rc dcp_pro_prof_absorb(struct dcp_pro_prof *p,
                                struct dcp_pro_model const *m)
{
    if (p->nuclt != pro_model_nuclt(m))
        return error(DCP_ILLEGALARG, "Different nucleotide alphabets.");

    if (p->amino != pro_model_amino(m))
        return error(DCP_ILLEGALARG, "Different amino alphabets.");

    struct pro_model_summary s = pro_model_summary(m);

    if (imm_hmm_reset_dp(s.null.hmm, imm_super(s.null.R), &p->null.dp))
        return error(DCP_RUNTIMEERROR, "failed to hmm_reset");

    if (imm_hmm_reset_dp(s.alt.hmm, imm_super(s.alt.T), &p->alt.dp))
        return error(DCP_RUNTIMEERROR, "failed to hmm_reset");

    p->null.R = imm_state_idx(imm_super(s.null.R));

    p->alt.S = imm_state_idx(imm_super(s.alt.S));
    p->alt.N = imm_state_idx(imm_super(s.alt.N));
    p->alt.B = imm_state_idx(imm_super(s.alt.B));
    p->alt.E = imm_state_idx(imm_super(s.alt.E));
    p->alt.J = imm_state_idx(imm_super(s.alt.J));
    p->alt.C = imm_state_idx(imm_super(s.alt.C));
    p->alt.T = imm_state_idx(imm_super(s.alt.T));
    return DCP_SUCCESS;
}

struct dcp_prof *dcp_pro_prof_super(struct dcp_pro_prof *pro)
{
    return &pro->super;
}

void dcp_pro_prof_state_name(unsigned id, char name[IMM_STATE_NAME_SIZE])
{
    pro_model_state_name(id, name);
}

void dcp_pro_prof_sample(struct dcp_pro_prof *p, unsigned seed,
                         unsigned core_size)
{
    assert(core_size >= 2);
    struct imm_rnd rnd = imm_rnd(seed);

    imm_float lprobs[IMM_AMINO_SIZE];

    imm_lprob_sample(&rnd, IMM_AMINO_SIZE, lprobs);
    imm_lprob_normalize(IMM_AMINO_SIZE, lprobs);

    struct dcp_pro_model model;
    dcp_pro_model_init(&model, p->amino, p->nuclt, p->cfg, lprobs);

    int rc = (int)dcp_pro_model_setup(&model, core_size);

    for (unsigned i = 0; i < core_size; ++i)
    {
        imm_lprob_sample(&rnd, IMM_AMINO_SIZE, lprobs);
        imm_lprob_normalize(IMM_AMINO_SIZE, lprobs);
        rc += dcp_pro_model_add_node(&model, lprobs);
    }

    for (unsigned i = 0; i < core_size + 1; ++i)
    {
        struct dcp_pro_trans t;
        imm_lprob_sample(&rnd, DCP_PRO_TRANS_SIZE, t.data);
        if (i == 0) t.DD = IMM_LPROB_ZERO;
        if (i == core_size)
        {
            t.MD = IMM_LPROB_ZERO;
            t.DD = IMM_LPROB_ZERO;
        }
        imm_lprob_normalize(DCP_PRO_TRANS_SIZE, t.data);
        rc += dcp_pro_model_add_trans(&model, t);
    }

    rc += dcp_pro_prof_absorb(p, &model);
    dcp_del(&model);

    assert(!rc);
}

void dcp_pro_prof_write_dot(struct dcp_pro_prof const *p, FILE *restrict fp)
{
    imm_dp_write_dot(&p->alt.dp, fp, pro_model_state_name);
}

static void del(struct dcp_prof *prof)
{
    if (prof)
    {
        struct dcp_pro_prof *p = (struct dcp_pro_prof *)prof;
        imm_del(&p->null.dp);
        imm_del(&p->alt.dp);
    }
}

enum dcp_rc pro_prof_read(struct dcp_pro_prof *prof, FILE *restrict fd)
{
    if (imm_dp_read(&prof->null.dp, fd)) return DCP_RUNTIMEERROR;
    if (imm_dp_read(&prof->alt.dp, fd)) return DCP_RUNTIMEERROR;
    return DCP_SUCCESS;
}

enum dcp_rc pro_prof_write(struct dcp_pro_prof const *prof, FILE *restrict fd)
{
    if (imm_dp_write(&prof->null.dp, fd)) return DCP_RUNTIMEERROR;
    if (imm_dp_write(&prof->alt.dp, fd)) return DCP_RUNTIMEERROR;
    return DCP_SUCCESS;
}
