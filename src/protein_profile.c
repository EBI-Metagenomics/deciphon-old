#include "protein_profile.h"
#include "cmp/cmp.h"
#include "imm/imm.h"
#include "logger.h"
#include "metadata.h"
#include "profile.h"
#include "profile_types.h"
#include "protein_model.h"
#include "protein_profile.h"
#include "third-party/xrandom.h"
#include <assert.h>
#include <stdlib.h>

static void del(struct profile *prof)
{
    if (prof)
    {
        struct protein_profile *p = (struct protein_profile *)prof;
        free(p->alt.match_ndists);
        imm_del(&p->null.dp);
        imm_del(&p->alt.dp);
    }
}

static enum rc alloc_match_nuclt_dists(struct protein_profile *prof)
{
    size_t size = prof->core_size * sizeof *prof->alt.match_ndists;
    void *ptr = realloc(prof->alt.match_ndists, size);
    if (!ptr && size > 0)
    {
        free(prof->alt.match_ndists);
        return error(RC_OUTOFMEM, "failed to alloc nuclt dists");
    }
    prof->alt.match_ndists = ptr;
    return RC_DONE;
}

static enum rc read(struct profile *prof, struct cmp_ctx_s *cmp)
{
    struct protein_profile *p = (struct protein_profile *)prof;
    FILE *fd = cmp_file(cmp);
    if (imm_dp_read(&p->null.dp, fd)) return RC_FAIL;
    if (imm_dp_read(&p->alt.dp, fd)) return RC_FAIL;

    uint16_t core_size = 0;
    if (!cmp_read_u16(cmp, &core_size))
        return error(RC_IOERROR, "failed to read core size");
    if (core_size > PROTEIN_MODEL_CORE_SIZE_MAX)
        return error(RC_PARSEERROR, "profile is too long");
    p->core_size = core_size;

    uint32_t u32 = (uint32_t)core_size;
    if (!cmp_read_cstr(cmp, p->consensus, &u32))
        return error(RC_IOERROR, "failed to read consensus");

    enum rc rc = alloc_match_nuclt_dists(p);
    if (rc) return rc;

    if ((rc = nuclt_dist_read(&p->null.ndist, cmp))) return rc;

    if ((rc = nuclt_dist_read(&p->alt.insert_ndist, cmp))) return rc;

    for (unsigned i = 0; i < p->core_size; ++i)
    {
        if ((rc = nuclt_dist_read(p->alt.match_ndists + i, cmp))) return rc;
        nuclt_dist_init(p->alt.match_ndists + i, p->code->nuclt);
    }
    return RC_DONE;
}

static struct imm_dp const *null_dp(struct profile const *prof)
{
    struct protein_profile *p = (struct protein_profile *)prof;
    return &p->null.dp;
}

static struct imm_dp const *alt_dp(struct profile const *prof)
{
    struct protein_profile *p = (struct protein_profile *)prof;
    return &p->alt.dp;
}

static struct profile_vtable vtable = {PROFILE_PROTEIN, del, read, null_dp,
                                       alt_dp};

void protein_profile_init(struct protein_profile *p,
                          struct imm_amino const *amino,
                          struct imm_nuclt_code const *code,
                          struct protein_cfg cfg)
{
    struct imm_nuclt const *nuclt = code->nuclt;
    profile_init(&p->super, &code->super, metadata_unset(), vtable);
    p->code = code;
    p->amino = amino;
    p->cfg = cfg;
    p->eps = imm_frame_epsilon(cfg.epsilon);
    p->core_size = 0;
    p->consensus[0] = '\0';
    imm_dp_init(&p->null.dp, &code->super);
    imm_dp_init(&p->alt.dp, &code->super);
    nuclt_dist_init(&p->null.ndist, nuclt);
    nuclt_dist_init(&p->alt.insert_ndist, nuclt);
    p->alt.match_ndists = NULL;
}

enum rc protein_profile_setup(struct protein_profile *prof, unsigned seq_size,
                              bool multi_hits, bool hmmer3_compat)
{
    if (seq_size == 0) return error(RC_ILLEGALARG, "sequence cannot be empty");

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

    struct protein_xtrans t;

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
    return RC_DONE;
}

enum rc protein_profile_absorb(struct protein_profile *p,
                               struct protein_model const *m)
{
    if (p->code->nuclt != protein_model_nuclt(m))
        return error(RC_ILLEGALARG, "Different nucleotide alphabets.");

    if (p->amino != protein_model_amino(m))
        return error(RC_ILLEGALARG, "Different amino alphabets.");

    struct protein_model_summary s = protein_model_summary(m);

    if (imm_hmm_reset_dp(s.null.hmm, imm_super(s.null.R), &p->null.dp))
        return error(RC_FAIL, "failed to hmm_reset");

    if (imm_hmm_reset_dp(s.alt.hmm, imm_super(s.alt.T), &p->alt.dp))
        return error(RC_FAIL, "failed to hmm_reset");

    p->core_size = m->core_size;
    memcpy(p->consensus, m->consensus, m->core_size + 1);
    enum rc rc = alloc_match_nuclt_dists(p);
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
    return RC_DONE;
}

unsigned protein_profile_state_name(unsigned id, char name[IMM_STATE_NAME_SIZE])
{
    return protein_state_name(id, name);
}

enum rc protein_profile_sample(struct protein_profile *p, unsigned seed,
                               unsigned core_size)
{
    assert(core_size >= 2);
    p->core_size = core_size;
    struct imm_rnd rnd = imm_rnd(seed);

    imm_float lprobs[IMM_AMINO_SIZE];

    imm_lprob_sample(&rnd, IMM_AMINO_SIZE, lprobs);
    imm_lprob_normalize(IMM_AMINO_SIZE, lprobs);

    struct protein_model model;
    protein_model_init(&model, p->amino, p->code, p->cfg, lprobs);

    enum rc rc = RC_DONE;

    if ((rc = protein_model_setup(&model, core_size))) goto cleanup;

    for (unsigned i = 0; i < core_size; ++i)
    {
        imm_lprob_sample(&rnd, IMM_AMINO_SIZE, lprobs);
        imm_lprob_normalize(IMM_AMINO_SIZE, lprobs);
        if ((rc = protein_model_add_node(&model, lprobs, '-'))) goto cleanup;
    }

    for (unsigned i = 0; i < core_size + 1; ++i)
    {
        struct protein_trans t;
        imm_lprob_sample(&rnd, PROTEIN_TRANS_SIZE, t.data);
        if (i == 0) t.DD = IMM_LPROB_ZERO;
        if (i == core_size)
        {
            t.MD = IMM_LPROB_ZERO;
            t.DD = IMM_LPROB_ZERO;
        }
        imm_lprob_normalize(PROTEIN_TRANS_SIZE, t.data);
        if ((rc = protein_model_add_trans(&model, t))) goto cleanup;
    }

    rc = protein_profile_absorb(p, &model);

cleanup:
    protein_model_del(&model);
    return rc;
}

enum rc protein_profile_decode(struct protein_profile const *prof,
                               struct imm_seq const *seq, unsigned state_id,
                               struct imm_codon *codon)
{
    assert(!protein_state_is_mute(state_id));

    struct nuclt_dist const *nucltd = NULL;
    if (protein_state_is_insert(state_id))
    {
        nucltd = &prof->alt.insert_ndist;
    }
    else if (protein_state_is_match(state_id))
    {
        unsigned idx = protein_state_idx(state_id);
        nucltd = prof->alt.match_ndists + idx;
    }
    else
        nucltd = &prof->null.ndist;

    struct imm_frame_cond cond = {prof->eps, &nucltd->nucltp, &nucltd->codonm};

    if (imm_lprob_is_nan(imm_frame_cond_decode(&cond, seq, codon)))
        return error(RC_ILLEGALARG, "failed to decode sequence");

    return RC_DONE;
}

void protein_profile_write_dot(struct protein_profile const *p,
                               FILE *restrict fp)
{
    imm_dp_write_dot(&p->alt.dp, fp, protein_state_name);
}

enum rc protein_profile_write(struct protein_profile const *prof,
                              struct cmp_ctx_s *cmp)
{
    FILE *fd = cmp_file(cmp);
    if (imm_dp_write(&prof->null.dp, fd)) return RC_FAIL;
    if (imm_dp_write(&prof->alt.dp, fd)) return RC_FAIL;

    if (!cmp_write_u16(cmp, (uint16_t)prof->core_size))
        return error(RC_IOERROR, "failed to write core size");

    if (!cmp_write_str(cmp, prof->consensus, prof->core_size))
        return error(RC_IOERROR, "failed to write consensus");

    enum rc rc = nuclt_dist_write(&prof->null.ndist, cmp);
    if (rc) return rc;

    if ((rc = nuclt_dist_write(&prof->alt.insert_ndist, cmp))) return rc;

    for (unsigned i = 0; i < prof->core_size; ++i)
    {
        if ((rc = nuclt_dist_write(prof->alt.match_ndists + i, cmp))) return rc;
    }
    return RC_DONE;
}
