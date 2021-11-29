#include "dcp/pro_prof.h"
#include "dcp/cmp.h"
#include "dcp/generics.h"
#include "dcp/prof.h"
#include "dcp/rc.h"
#include "error.h"
#include "imm/imm.h"
#include "meta.h"
#include "pro_model.h"
#include "pro_prof.h"
#include "prof.h"
#include "third-party/cmp.h"
#include "third-party/xrandom.h"
#include "xcmp.h"
#include <assert.h>
#include <stdlib.h>

static void del(struct dcp_prof *prof);

void dcp_pro_prof_init(struct dcp_pro_prof *p, struct imm_amino const *amino,
                       struct imm_nuclt_code const *code,
                       struct dcp_pro_cfg cfg)
{
    struct dcp_prof_vtable vtable = {del, DCP_PRO_PROFILE};
    struct imm_nuclt const *nuclt = code->nuclt;
    profile_init(&p->super, &code->super, meta_unset, vtable);
    p->code = code;
    p->amino = amino;
    p->cfg = cfg;
    p->eps = imm_frame_epsilon(cfg.epsilon);
    p->core_size = 0;
    p->consensus[0] = '\0';
    imm_dp_init(&p->null.dp, &code->super);
    imm_dp_init(&p->alt.dp, &code->super);
    dcp_nuclt_dist_init(&p->null.ndist, nuclt);
    dcp_nuclt_dist_init(&p->alt.insert_ndist, nuclt);
    p->alt.match_ndists = NULL;
}

static enum dcp_rc alloc_match_nuclt_dists(struct dcp_pro_prof *prof)
{
    size_t size = prof->core_size * sizeof *prof->alt.match_ndists;
    void *ptr = realloc(prof->alt.match_ndists, size);
    if (!ptr && size > 0)
    {
        free(prof->alt.match_ndists);
        return error(DCP_OUTOFMEM, "failed to alloc nuclt dists");
    }
    prof->alt.match_ndists = ptr;
    return DCP_DONE;
}

enum dcp_rc dcp_pro_prof_setup(struct dcp_pro_prof *prof, unsigned seq_size,
                               bool multi_hits, bool hmmer3_compat)
{
    if (seq_size == 0) return error(DCP_ILLEGALARG, "sequence cannot be empty");

    imm_float L = (imm_float)seq_size;

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

    struct imm_dp *dp = &prof->null.dp;
    unsigned R = prof->null.R;
    imm_dp_change_trans(dp, imm_dp_trans_idx(dp, R, R), t.RR);

    dp = &prof->alt.dp;
    unsigned S = prof->alt.S;
    unsigned N = prof->alt.N;
    unsigned B = prof->alt.B;
    unsigned E = prof->alt.E;
    unsigned J = prof->alt.J;
    unsigned C = prof->alt.C;
    unsigned T = prof->alt.T;

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
    return DCP_DONE;
}

enum dcp_rc dcp_pro_prof_absorb(struct dcp_pro_prof *p,
                                struct dcp_pro_model const *m)
{
    if (p->code->nuclt != pro_model_nuclt(m))
        return error(DCP_ILLEGALARG, "Different nucleotide alphabets.");

    if (p->amino != pro_model_amino(m))
        return error(DCP_ILLEGALARG, "Different amino alphabets.");

    struct pro_model_summary s = pro_model_summary(m);

    if (imm_hmm_reset_dp(s.null.hmm, imm_super(s.null.R), &p->null.dp))
        return error(DCP_FAIL, "failed to hmm_reset");

    if (imm_hmm_reset_dp(s.alt.hmm, imm_super(s.alt.T), &p->alt.dp))
        return error(DCP_FAIL, "failed to hmm_reset");

    p->core_size = m->core_size;
    memcpy(p->consensus, m->consensus, m->core_size + 1);
    enum dcp_rc rc = alloc_match_nuclt_dists(p);
    if (rc) return rc;

    p->null.ndist = m->null.nucltd;

    for (unsigned i = 0; i < m->core_size; ++i)
        p->alt.match_ndists[i] = m->alt.nodes[i].match.nucltd;

    p->alt.insert_ndist = m->alt.insert.nucltd;

    p->null.R = imm_state_idx(imm_super(s.null.R));

    p->alt.S = imm_state_idx(imm_super(s.alt.S));
    p->alt.N = imm_state_idx(imm_super(s.alt.N));
    p->alt.B = imm_state_idx(imm_super(s.alt.B));
    p->alt.E = imm_state_idx(imm_super(s.alt.E));
    p->alt.J = imm_state_idx(imm_super(s.alt.J));
    p->alt.C = imm_state_idx(imm_super(s.alt.C));
    p->alt.T = imm_state_idx(imm_super(s.alt.T));
    return DCP_DONE;
}

struct dcp_prof *dcp_pro_prof_super(struct dcp_pro_prof *pro)
{
    return &pro->super;
}

void dcp_pro_prof_state_name(unsigned id, char name[IMM_STATE_NAME_SIZE])
{
    dcp_pro_state_name(id, name);
}

enum dcp_rc dcp_pro_prof_sample(struct dcp_pro_prof *p, unsigned seed,
                                unsigned core_size)
{
    assert(core_size >= 2);
    p->core_size = core_size;
    struct imm_rnd rnd = imm_rnd(seed);

    imm_float lprobs[IMM_AMINO_SIZE];

    imm_lprob_sample(&rnd, IMM_AMINO_SIZE, lprobs);
    imm_lprob_normalize(IMM_AMINO_SIZE, lprobs);

    struct dcp_pro_model model;
    dcp_pro_model_init(&model, p->amino, p->code, p->cfg, lprobs);

    enum dcp_rc rc = DCP_DONE;

    if ((rc = dcp_pro_model_setup(&model, core_size))) goto cleanup;

    for (unsigned i = 0; i < core_size; ++i)
    {
        imm_lprob_sample(&rnd, IMM_AMINO_SIZE, lprobs);
        imm_lprob_normalize(IMM_AMINO_SIZE, lprobs);
        if ((rc = dcp_pro_model_add_node(&model, lprobs, '-'))) goto cleanup;
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
        if ((rc = dcp_pro_model_add_trans(&model, t))) goto cleanup;
    }

    rc = dcp_pro_prof_absorb(p, &model);

cleanup:
    dcp_del(&model);
    return rc;
}

enum dcp_rc dcp_pro_prof_decode(struct dcp_pro_prof const *prof,
                                struct imm_seq const *seq, unsigned state_id,
                                struct imm_codon *codon)
{
    assert(!dcp_pro_state_is_mute(state_id));

    struct dcp_nuclt_dist const *nucltd = NULL;
    if (dcp_pro_state_is_insert(state_id))
    {
        nucltd = &prof->alt.insert_ndist;
    }
    else if (dcp_pro_state_is_match(state_id))
    {
        unsigned idx = dcp_pro_state_idx(state_id);
        nucltd = prof->alt.match_ndists + idx;
    }
    else
        nucltd = &prof->null.ndist;

    struct imm_frame_cond cond = {prof->eps, &nucltd->nucltp, &nucltd->codonm};

    if (imm_lprob_is_nan(imm_frame_cond_decode(&cond, seq, codon)))
        return error(DCP_ILLEGALARG, "failed to decode sequence");

    return DCP_DONE;
}

void dcp_pro_prof_write_dot(struct dcp_pro_prof const *p, FILE *restrict fp)
{
    imm_dp_write_dot(&p->alt.dp, fp, dcp_pro_state_name);
}

static void del(struct dcp_prof *prof)
{
    if (prof)
    {
        struct dcp_pro_prof *p = (struct dcp_pro_prof *)prof;
        free(p->alt.match_ndists);
        imm_del(&p->null.dp);
        imm_del(&p->alt.dp);
    }
}

enum dcp_rc pro_prof_read(struct dcp_pro_prof *prof, struct cmp_ctx_s *cmp)
{
    FILE *fd = xcmp_fp(cmp);
    if (imm_dp_read(&prof->null.dp, fd)) return DCP_FAIL;
    if (imm_dp_read(&prof->alt.dp, fd)) return DCP_FAIL;

    uint16_t core_size = 0;
    if (!cmp_read_u16(cmp, &core_size))
        return error(DCP_IOERROR, "failed to read core size");
    if (core_size > DCP_PRO_MODEL_CORE_SIZE_MAX)
        return error(DCP_PARSEERROR, "profile is too long");
    prof->core_size = core_size;

    uint32_t u32 = (uint32_t)core_size + 1;
    if (!cmp_read_str(cmp, prof->consensus, &u32))
        return error(DCP_IOERROR, "failed to read consensus");

    enum dcp_rc rc = alloc_match_nuclt_dists(prof);
    if (rc) return rc;

    if ((rc = dcp_nuclt_dist_read(&prof->null.ndist, cmp))) return rc;

    if ((rc = dcp_nuclt_dist_read(&prof->alt.insert_ndist, cmp))) return rc;

    for (unsigned i = 0; i < prof->core_size; ++i)
    {
        if ((rc = dcp_nuclt_dist_read(prof->alt.match_ndists + i, cmp)))
            return rc;
        dcp_nuclt_dist_init(prof->alt.match_ndists + i, prof->code->nuclt);
    }
    return DCP_DONE;
}

enum dcp_rc pro_prof_write(struct dcp_pro_prof const *prof,
                           struct cmp_ctx_s *cmp)
{
    FILE *fd = xcmp_fp(cmp);
    if (imm_dp_write(&prof->null.dp, fd)) return DCP_FAIL;
    if (imm_dp_write(&prof->alt.dp, fd)) return DCP_FAIL;

    if (!cmp_write_u16(cmp, (uint16_t)prof->core_size))
        return error(DCP_IOERROR, "failed to write core size");

    if (!cmp_write_str(cmp, prof->consensus, prof->core_size))
        return error(DCP_IOERROR, "failed to write consensus");

    enum dcp_rc rc = dcp_nuclt_dist_write(&prof->null.ndist, cmp);
    if (rc) return rc;

    if ((rc = dcp_nuclt_dist_write(&prof->alt.insert_ndist, cmp))) return rc;

    for (unsigned i = 0; i < prof->core_size; ++i)
    {
        if ((rc = dcp_nuclt_dist_write(prof->alt.match_ndists + i, cmp)))
            return rc;
    }
    return DCP_DONE;
}
