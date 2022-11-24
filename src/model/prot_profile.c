#include "model/prot_profile.h"
#include "expect.h"
#include "imm/imm.h"
#include "lite_pack/file/file.h"
#include "lite_pack/lite_pack.h"
#include "logy.h"
#include "model/profile.h"
#include "model/profile_typeid.h"
#include "model/prot_model.h"
#include "model/prot_profile.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

static void del(struct profile *prof)
{
    if (prof)
    {
        struct prot_profile *p = (struct prot_profile *)prof;
        free(p->alt.match_ndists);
        imm_del(&p->null.dp);
        imm_del(&p->alt.dp);
    }
}

static enum rc alloc_match_nuclt_dists(struct prot_profile *prof)
{
    size_t size = prof->core_size * sizeof *prof->alt.match_ndists;
    void *ptr = realloc(prof->alt.match_ndists, size);
    if (!ptr && size > 0)
    {
        free(prof->alt.match_ndists);
        return enomem("alloc nuclt dists");
    }
    prof->alt.match_ndists = ptr;
    return RC_OK;
}

static enum rc unpack(struct profile *prof, struct lip_file *file)
{
    struct prot_profile *p = (struct prot_profile *)prof;
    unsigned size = 0;
    if (!lip_read_map_size(file, &size)) eio("read profile map size");
    assert(size == 16);

    if (!expect_map_key(file, "accession")) return eio("read key");
    if (!lip_read_cstr(file, PROFILE_ACC_SIZE, prof->accession))
        return eio("write accession");

    if (!expect_map_key(file, "null")) return eio("skip key");
    if (imm_dp_unpack(&p->null.dp, file)) return RC_EFAIL;

    if (!expect_map_key(file, "alt")) return eio("skip key");
    if (imm_dp_unpack(&p->alt.dp, file)) return RC_EFAIL;

    if (!expect_map_key(file, "core_size")) return eio("skip key");
    if (!lip_read_int(file, &size)) return eio("read core size");
    if (size > PROTEIN_MODEL_CORE_SIZE_MAX) return eio("profile is too long");
    p->core_size = size;

    if (!expect_map_key(file, "consensus")) return eio("skip key");
    size = p->core_size;
    if (!lip_read_cstr(file, PROTEIN_MODEL_CORE_SIZE_MAX, p->consensus))
        return eio("read consensus");

    unsigned s = 0;

    if (!expect_map_key(file, "R")) return eio("skip key");
    if (!lip_read_int(file, &s)) return eio("read R state");
    p->null.R = (unsigned)s;

    if (!expect_map_key(file, "S")) return eio("skip key");
    if (!lip_read_int(file, &s)) return eio("read S state");
    p->alt.S = (unsigned)s;

    if (!expect_map_key(file, "N")) return eio("skip key");
    if (!lip_read_int(file, &s)) return eio("read N state");
    p->alt.N = (unsigned)s;

    if (!expect_map_key(file, "B")) return eio("skip key");
    if (!lip_read_int(file, &s)) return eio("read B state");
    p->alt.B = (unsigned)s;

    if (!expect_map_key(file, "E")) return eio("skip key");
    if (!lip_read_int(file, &s)) return eio("read E state");
    p->alt.E = (unsigned)s;

    if (!expect_map_key(file, "J")) return eio("skip key");
    if (!lip_read_int(file, &s)) return eio("read J state");
    p->alt.J = (unsigned)s;

    if (!expect_map_key(file, "C")) return eio("skip key");
    if (!lip_read_int(file, &s)) return eio("read C state");
    p->alt.C = (unsigned)s;

    if (!expect_map_key(file, "T")) return eio("skip key");
    if (!lip_read_int(file, &s)) return eio("read T state");
    p->alt.T = (unsigned)s;

    enum rc rc = alloc_match_nuclt_dists(p);
    if (rc) return rc;

    if (!expect_map_key(file, "null_ndist")) return eio("skip key");
    if ((rc = nuclt_dist_unpack(&p->null.ndist, file))) return rc;

    if (!expect_map_key(file, "alt_insert_ndist")) return eio("skip key");
    if ((rc = nuclt_dist_unpack(&p->alt.insert_ndist, file))) return rc;

    if (!expect_map_key(file, "alt_match_ndist")) return eio("skip key");
    if (!lip_read_array_size(file, &size)) return eio("read size");
    assert(size == p->core_size);
    for (unsigned i = 0; i < p->core_size; ++i)
    {
        if ((rc = nuclt_dist_unpack(p->alt.match_ndists + i, file))) return rc;
        nuclt_dist_init(p->alt.match_ndists + i, p->code->nuclt);
    }
    return RC_OK;
}

static struct imm_dp const *null_dp(struct profile const *prof)
{
    struct prot_profile *p = (struct prot_profile *)prof;
    return &p->null.dp;
}

static struct imm_dp const *alt_dp(struct profile const *prof)
{
    struct prot_profile *p = (struct prot_profile *)prof;
    return &p->alt.dp;
}

static struct profile_vtable vtable = {PROFILE_PROTEIN, del, unpack, null_dp,
                                       alt_dp};

void prot_profile_init(struct prot_profile *p, char const *accession,
                       struct imm_amino const *amino,
                       struct imm_nuclt_code const *code, struct prot_cfg cfg)
{
    struct imm_nuclt const *nuclt = code->nuclt;
    profile_init(&p->super, accession, &code->super, vtable, prot_state_name);
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

enum rc prot_profile_setup(struct prot_profile *prof, unsigned seq_size,
                           bool multi_hits, bool hmmer3_compat)
{
    if (seq_size == 0) return einval("sequence cannot be empty");

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

    struct prot_xtrans t;

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

    imm_dp_change_trans(dp, imm_dp_trans_idx(dp, E, B), t.EJ + t.JB);
    imm_dp_change_trans(dp, imm_dp_trans_idx(dp, E, J), t.EJ + t.JJ);
    imm_dp_change_trans(dp, imm_dp_trans_idx(dp, J, J), t.JJ);
    imm_dp_change_trans(dp, imm_dp_trans_idx(dp, J, B), t.JB);
    return RC_OK;
}

enum rc prot_profile_absorb(struct prot_profile *p, struct prot_model const *m)
{
    if (p->code->nuclt != prot_model_nuclt(m))
        return einval("Different nucleotide alphabets.");

    if (p->amino != prot_model_amino(m))
        return einval("Different amino alphabets.");

    struct prot_model_summary s = prot_model_summary(m);

    if (imm_hmm_reset_dp(s.null.hmm, imm_super(s.null.R), &p->null.dp))
        return efail("failed to hmm_reset");

    if (imm_hmm_reset_dp(s.alt.hmm, imm_super(s.alt.T), &p->alt.dp))
        return efail("failed to hmm_reset");

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
    return RC_OK;
}

enum rc prot_profile_sample(struct prot_profile *p, unsigned seed,
                            unsigned core_size)
{
    assert(core_size >= 2);
    p->core_size = core_size;
    struct imm_rnd rnd = imm_rnd(seed);

    imm_float lprobs[IMM_AMINO_SIZE];

    imm_lprob_sample(&rnd, IMM_AMINO_SIZE, lprobs);
    imm_lprob_normalize(IMM_AMINO_SIZE, lprobs);

    struct prot_model model;
    prot_model_init(&model, p->amino, p->code, p->cfg, lprobs);

    enum rc rc = RC_OK;

    if ((rc = prot_model_setup(&model, core_size))) goto cleanup;

    for (unsigned i = 0; i < core_size; ++i)
    {
        imm_lprob_sample(&rnd, IMM_AMINO_SIZE, lprobs);
        imm_lprob_normalize(IMM_AMINO_SIZE, lprobs);
        if ((rc = prot_model_add_node(&model, lprobs, '-'))) goto cleanup;
    }

    for (unsigned i = 0; i < core_size + 1; ++i)
    {
        struct prot_trans t;
        imm_lprob_sample(&rnd, PROTEIN_TRANS_SIZE, t.data);
        if (i == 0) t.DD = IMM_LPROB_ZERO;
        if (i == core_size)
        {
            t.MD = IMM_LPROB_ZERO;
            t.DD = IMM_LPROB_ZERO;
        }
        imm_lprob_normalize(PROTEIN_TRANS_SIZE, t.data);
        if ((rc = prot_model_add_trans(&model, t))) goto cleanup;
    }

    rc = prot_profile_absorb(p, &model);

cleanup:
    prot_model_del(&model);
    return rc;
}

enum rc prot_profile_decode(struct prot_profile const *prof,
                            struct imm_seq const *seq, unsigned state_id,
                            struct imm_codon *codon)
{
    assert(!prot_state_is_mute(state_id));

    struct nuclt_dist const *nucltd = NULL;
    if (prot_state_is_insert(state_id))
    {
        nucltd = &prof->alt.insert_ndist;
    }
    else if (prot_state_is_match(state_id))
    {
        unsigned idx = prot_state_idx(state_id);
        nucltd = prof->alt.match_ndists + idx;
    }
    else
        nucltd = &prof->null.ndist;

    struct imm_frame_cond cond = {prof->eps, &nucltd->nucltp, &nucltd->codonm};

    if (imm_lprob_is_nan(imm_frame_cond_decode(&cond, seq, codon)))
        return einval("failed to decode sequence");

    return RC_OK;
}

void prot_profile_write_dot(struct prot_profile const *p, FILE *fp)
{
    imm_dp_write_dot(&p->alt.dp, fp, prot_state_name);
}

enum rc prot_profile_pack(struct prot_profile const *prof,
                          struct lip_file *file)
{
    if (!lip_write_map_size(file, 16)) return eio("write profile map size");

    if (!lip_write_cstr(file, "accession")) return eio("write acession key");
    if (!lip_write_cstr(file, prof->super.accession))
        return eio("write accession");

    if (!lip_write_cstr(file, "null")) return eio("write null key");
    if (imm_dp_pack(&prof->null.dp, file)) return RC_EFAIL;

    if (!lip_write_cstr(file, "alt")) return eio("write alt key");
    if (imm_dp_pack(&prof->alt.dp, file)) return RC_EFAIL;

    if (!lip_write_cstr(file, "core_size")) return eio("write core_size key");
    if (!lip_write_int(file, prof->core_size)) return eio("write core_size");

    if (!lip_write_cstr(file, "consensus")) return eio("write consensus key");
    if (!lip_write_cstr(file, prof->consensus)) return eio("write consensus");

    if (!lip_write_cstr(file, "R")) return eio("write R state key");
    if (!lip_write_int(file, prof->null.R)) return eio("write R state");

    if (!lip_write_cstr(file, "S")) return eio("write S state key");
    if (!lip_write_int(file, prof->alt.S)) return eio("write S state");

    if (!lip_write_cstr(file, "N")) return eio("write N state key");
    if (!lip_write_int(file, prof->alt.N)) return eio("write N state");

    if (!lip_write_cstr(file, "B")) return eio("write B state key");
    if (!lip_write_int(file, prof->alt.B)) return eio("write B state");

    if (!lip_write_cstr(file, "E")) return eio("write E state key");
    if (!lip_write_int(file, prof->alt.E)) return eio("write E state");

    if (!lip_write_cstr(file, "J")) return eio("write J state key");
    if (!lip_write_int(file, prof->alt.J)) return eio("write J state");

    if (!lip_write_cstr(file, "C")) return eio("write C state key");
    if (!lip_write_int(file, prof->alt.C)) return eio("write C state");

    if (!lip_write_cstr(file, "T")) return eio("write T state key");
    if (!lip_write_int(file, prof->alt.T)) return eio("write T state");

    if (!lip_write_cstr(file, "null_ndist")) return eio("write null_ndist key");
    enum rc rc = nuclt_dist_pack(&prof->null.ndist, file);
    if (rc) return rc;

    if (!lip_write_cstr(file, "alt_insert_ndist"))
        return eio("write alt_insert_ndist key");
    if ((rc = nuclt_dist_pack(&prof->alt.insert_ndist, file))) return rc;

    if (!lip_write_cstr(file, "alt_match_ndist"))
        return eio("write alt_match_ndist key");
    if (!lip_write_array_size(file, prof->core_size))
        return eio("write array length");
    for (unsigned i = 0; i < prof->core_size; ++i)
    {
        if ((rc = nuclt_dist_pack(prof->alt.match_ndists + i, file))) return rc;
    }
    return RC_OK;
}
