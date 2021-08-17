#include "dcp/pro/profile.h"
#include "dcp/generics.h"
#include "dcp/profile.h"
#include "imm/imm.h"
#include "pro/model.h"
#include "profile.h"
#include "support.h"
#include "third-party/xrandom.h"
#include <assert.h>

static int read(struct dcp_profile *prof, FILE *restrict fd);

static int write(struct dcp_profile const *prof, FILE *restrict fd);

static void del(struct dcp_profile *prof);

void dcp_pro_profile_init(struct dcp_pro_profile *p, struct dcp_pro_cfg cfg)
{
    struct dcp_profile_vtable vtable = {read, write, del, DCP_PROTEIN_PROFILE,
                                        p};
    profile_init(&p->super, imm_super(cfg.nuclt), dcp_meta(NULL, NULL), vtable);
    p->cfg = cfg;
    imm_dp_init(&p->null.dp, imm_super(cfg.nuclt));
    imm_dp_init(&p->alt.dp, imm_super(cfg.nuclt));
}

void dcp_pro_profile_setup(struct dcp_pro_profile *p, unsigned seq_len,
                           bool multihits, bool hmmer3_compat)
{
    IMM_BUG(seq_len == 0);
    imm_float L = (imm_float)seq_len;

    imm_float q = 0.0;
    imm_float log_q = IMM_LPROB_ZERO;

    if (multihits)
    {
        q = 0.5;
        log_q = imm_log(0.5);
    }

    imm_float lp = imm_log(L) - imm_log(L + 2 + q / (1 - q));
    imm_float l1p = imm_log(2 + q / (1 - q)) - imm_log(L + 2 + q / (1 - q));
    imm_float lr = imm_log(L) - imm_log(L + 1);

    struct dcp_pro_special_trans t;

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

int dcp_pro_profile_absorb(struct dcp_pro_profile *p,
                           struct dcp_pro_model const *m)
{
    int rc = IMM_SUCCESS;

    if (p->cfg.nuclt != pro_model_nuclt(m))
        return error(IMM_ILLEGALARG, "Different nucleotide alphabets.");

    if (p->cfg.amino != pro_model_amino(m))
        return error(IMM_ILLEGALARG, "Different amino alphabets.");

    struct pro_model_summary s = pro_model_summary(m);
    imm_hmm_reset_dp(s.null.hmm, imm_super(s.null.R), &p->null.dp);
    imm_hmm_reset_dp(s.alt.hmm, imm_super(s.alt.T), &p->alt.dp);

    p->null.R = imm_state_idx(imm_super(s.null.R));

    p->alt.S = imm_state_idx(imm_super(s.alt.S));
    p->alt.N = imm_state_idx(imm_super(s.alt.N));
    p->alt.B = imm_state_idx(imm_super(s.alt.B));
    p->alt.E = imm_state_idx(imm_super(s.alt.E));
    p->alt.J = imm_state_idx(imm_super(s.alt.J));
    p->alt.C = imm_state_idx(imm_super(s.alt.C));
    p->alt.T = imm_state_idx(imm_super(s.alt.T));
    return rc;
}

struct dcp_profile *dcp_pro_profile_super(struct dcp_pro_profile *pro)
{
    return &pro->super;
}

void dcp_pro_profile_state_name(unsigned id, char name[IMM_STATE_NAME_SIZE])
{
    pro_model_state_name(id, name);
}

void dcp_pro_profile_sample(struct dcp_pro_profile *p, unsigned seed,
                            unsigned core_size, enum dcp_entry_dist edist,
                            imm_float epsilon)
{
    IMM_BUG(core_size < 2);
    struct imm_rnd rnd = imm_rnd(seed);
    struct dcp_pro_cfg cfg = {
        .amino = &imm_amino_iupac,
        .nuclt = imm_super(imm_gc_dna()),
        .edist = edist,
        .epsilon = epsilon,
    };

    imm_float lprobs[IMM_AMINO_SIZE];
    imm_float lodds[IMM_AMINO_SIZE];

    imm_lprob_sample(&rnd, IMM_AMINO_SIZE, lprobs);
    imm_lprob_normalize(IMM_AMINO_SIZE, lprobs);
    imm_lprob_sample(&rnd, IMM_AMINO_SIZE, lodds);

    struct dcp_pro_model *model = dcp_pro_model_new(cfg, lprobs, lodds);

    int rc = dcp_pro_model_setup(model, core_size);

    for (unsigned i = 0; i < core_size; ++i)
    {
        imm_lprob_sample(&rnd, IMM_AMINO_SIZE, lprobs);
        imm_lprob_normalize(IMM_AMINO_SIZE, lprobs);
        rc += dcp_pro_model_add_node(model, lprobs);
    }

    for (unsigned i = 0; i < core_size + 1; ++i)
    {
        struct dcp_pro_trans t;
        imm_lprob_sample(&rnd, DCP_PRO_TRANS_SIZE, t.data);
        if (i == 0)
            t.DD = IMM_LPROB_ZERO;
        if (i == core_size)
        {
            t.MD = IMM_LPROB_ZERO;
            t.DD = IMM_LPROB_ZERO;
        }
        imm_lprob_normalize(DCP_PRO_TRANS_SIZE, t.data);
        rc += dcp_pro_model_add_trans(model, t);
    }

    dcp_pro_profile_init(p, cfg);
    rc += dcp_pro_profile_absorb(p, model);
    dcp_del(model);

    IMM_BUG(rc);
}

static int read(struct dcp_profile *prof, FILE *restrict fd)
{
    int rc = IMM_SUCCESS;
    struct dcp_pro_profile *p = prof->vtable.derived;

    if ((rc = imm_dp_read(&p->null.dp, fd)))
        return rc;

    if ((rc = imm_dp_read(&p->alt.dp, fd)))
        return rc;

    return rc;
}

static int write(struct dcp_profile const *prof, FILE *restrict fd)
{
    int rc = IMM_SUCCESS;
    struct dcp_pro_profile const *p = prof->vtable.derived;

    if ((rc = imm_dp_write(&p->null.dp, fd)))
        return rc;

    if ((rc = imm_dp_write(&p->alt.dp, fd)))
        return rc;

    return rc;
}

static void del(struct dcp_profile *prof)
{
    if (prof)
    {
        struct dcp_pro_profile *p = prof->vtable.derived;
        imm_del(&p->null.dp);
        imm_del(&p->alt.dp);
    }
}

static void sample_lprobs(struct xrandom *rnd, unsigned n, imm_float *lprobs)
{
    for (unsigned i = 0; i < n; ++i)
        lprobs[i] = imm_log(random_dbl(rnd));
}
