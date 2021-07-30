#include "dcp/pro_model.h"
#include "dcp/entry_dist.h"
#include "pro_model.h"
#include "support.h"
#include <limits.h>

struct node
{
    struct imm_frame_state M;
    struct imm_frame_state I;
    struct imm_mute_state D;
    struct imm_nuclt_lprob nucltp;
    struct imm_codon_marg codonm;
};

struct special_node
{
    struct
    {
        struct imm_frame_state R;
    } null;

    struct
    {
        struct imm_mute_state S;
        struct imm_frame_state N;
        struct imm_mute_state B;
        struct imm_mute_state E;
        struct imm_frame_state J;
        struct imm_frame_state C;
        struct imm_mute_state T;
    } alt;
};

struct dcp_pro_model
{
    struct dcp_pro_cfg cfg;
    unsigned core_size;
    struct special_node special_node;
    struct pro_model_special_trans special_trans;

    struct
    {
        imm_float lprobs[IMM_AMINO_SIZE];
        struct imm_nuclt_lprob nucltp;
        struct imm_codon_marg codonm;
        struct imm_hmm hmm;
    } null;

    struct
    {
        unsigned node_idx;
        struct node *nodes;
        unsigned trans_idx;
        struct dcp_pro_model_trans *trans;
        struct imm_hmm hmm;

        struct
        {
            struct imm_nuclt_lprob nucltp;
            struct imm_codon_marg codonm;
        } insert;
    } alt;
};

/* log(1./4.) */
#define LOGN2 (imm_float)(-1.3862943611198906)

static imm_float const uniform_nuclt_lprobs[IMM_NUCLT_SIZE] = {LOGN2, LOGN2,
                                                               LOGN2, LOGN2};

static void add_special_node(struct dcp_pro_model *m);

static void calc_occupancy(struct dcp_pro_model *m,
                           imm_float log_occ[static 1]);

static inline bool is_setup(struct dcp_pro_model *m)
{
    return m->core_size > 0;
}

static void init_special_trans(struct dcp_pro_model *m);

static inline bool is_ready(struct dcp_pro_model const *m)
{
    unsigned core_size = m->core_size;
    return m->alt.node_idx == core_size && m->alt.trans_idx == (core_size + 1);
}

/* Compute log(1 - p) given log(p). */
static inline imm_float log1_p(imm_float logp) { return log1p(-exp(logp)); }

static void init_delete(struct imm_mute_state *state, struct dcp_pro_model *m);

static void init_insert(struct imm_frame_state *state, struct dcp_pro_model *m);

static void init_match(struct imm_frame_state *state, struct dcp_pro_model *m,
                       struct imm_nuclt_lprob *nucltp,
                       struct imm_codon_marg *codonm);

static void new_special_node(struct dcp_pro_model *m);

static struct imm_codon_lprob setup_codonp(struct imm_amino const *amino,
                                           struct imm_amino_lprob const *aminop,
                                           struct imm_nuclt const *nuclt);

static int setup_distrs(struct imm_amino const *amino,
                        struct imm_nuclt const *nuclt,
                        imm_float const lprobs[IMM_AMINO_SIZE],
                        struct imm_nuclt_lprob *nucltp,
                        struct imm_codon_marg *codonm);

static void setup_entry_trans(struct dcp_pro_model *model);

static void setup_exit_trans(struct dcp_pro_model *model);

static struct imm_nuclt_lprob
setup_nucltp(struct imm_codon_lprob const *codonp);

static void setup_trans(struct dcp_pro_model *model);

int dcp_pro_model_add_node(struct dcp_pro_model *m,
                           imm_float const lprobs[IMM_AMINO_SIZE])
{
    if (!is_setup(m))
        return error(IMM_RUNTIMEERROR, "Must call dcp_pro_model_setup first.");

    if (m->alt.node_idx == m->core_size)
        return error(IMM_RUNTIMEERROR, "Reached limit of nodes.");

    imm_float lodds[IMM_AMINO_SIZE];
    for (unsigned i = 0; i < IMM_AMINO_SIZE; ++i)
        lodds[i] = lprobs[i] - m->null.lprobs[i];

    int rc = IMM_SUCCESS;
    struct node *n = m->alt.nodes + m->alt.node_idx;

    if ((rc = setup_distrs(m->cfg.amino, m->cfg.nuclt, lodds, &n->nucltp,
                           &n->codonm)))
        return rc;

    init_match(&n->M, m, &n->nucltp, &n->codonm);
    if ((rc = imm_hmm_add_state(&m->alt.hmm, imm_super(&n->M))))
        return rc;

    init_insert(&n->I, m);
    if ((rc = imm_hmm_add_state(&m->alt.hmm, imm_super(&n->I))))
        return rc;

    init_delete(&n->D, m);
    if ((rc = imm_hmm_add_state(&m->alt.hmm, imm_super(&n->D))))
        return rc;

    m->alt.node_idx++;

    if (is_ready(m))
        setup_trans(m);

    return rc;
}

int dcp_pro_model_add_trans(struct dcp_pro_model *m,
                            struct dcp_pro_model_trans trans)
{
    if (!is_setup(m))
        return error(IMM_RUNTIMEERROR, "Must call dcp_pro_model_setup first.");

    if (m->alt.trans_idx == m->core_size + 1)
        return error(IMM_RUNTIMEERROR, "Reached limit of transitions.");

    m->alt.trans[m->alt.trans_idx++] = trans;
    if (is_ready(m))
        setup_trans(m);
    return IMM_SUCCESS;
}

void dcp_pro_model_del(struct dcp_pro_model const *model)
{
    if (model)
    {
        free(model->alt.nodes);
        free(model->alt.trans);
        free((void *)model);
    }
}

struct dcp_pro_model *
dcp_pro_model_new(struct dcp_pro_cfg cfg,
                  imm_float const null_lprobs[IMM_AMINO_SIZE],
                  imm_float const ins_lodds[IMM_AMINO_SIZE])

{
    struct dcp_pro_model *m = xmalloc(sizeof(*m));
    m->cfg = cfg;
    m->core_size = 0;

    xmemcpy(m->null.lprobs, null_lprobs, sizeof(*null_lprobs) * IMM_AMINO_SIZE);

    imm_hmm_init(&m->null.hmm, imm_super(m->cfg.nuclt));

    if (setup_distrs(cfg.amino, cfg.nuclt, null_lprobs, &m->null.nucltp,
                     &m->null.codonm))
        goto cleanup;

    imm_hmm_init(&m->alt.hmm, imm_super(m->cfg.nuclt));

    if (setup_distrs(cfg.amino, cfg.nuclt, ins_lodds, &m->alt.insert.nucltp,
                     &m->alt.insert.codonm))
        goto cleanup;

    new_special_node(m);

    m->alt.node_idx = UINT_MAX;
    m->alt.nodes = NULL;
    m->alt.trans_idx = UINT_MAX;
    m->alt.trans = NULL;
    m->special_trans = pro_model_special_trans_init();

    return m;

cleanup:

    free(m->alt.nodes);
    free(m->alt.trans);
    free(m);
    return NULL;
}

int dcp_pro_model_setup(struct dcp_pro_model *m, unsigned core_size)
{
    if (core_size == 0)
        return error(IMM_ILLEGALARG, "`core_size` cannot be zero.");

    m->core_size = core_size;
    unsigned n = m->core_size;
    m->alt.node_idx = 0;
    m->alt.nodes = xrealloc(m->alt.nodes, n * sizeof(*m->alt.nodes));
    m->alt.trans_idx = 0;
    m->alt.trans = xrealloc(m->alt.trans, (n + 1) * sizeof(*m->alt.trans));
    imm_hmm_reset(&m->alt.hmm);
    imm_hmm_reset(&m->null.hmm);
    add_special_node(m);
    return IMM_SUCCESS;
}

struct pro_model_summary pro_model_summary(struct dcp_pro_model const *m)
{
    IMM_BUG(!is_ready(m));
    return (struct pro_model_summary){
        .null = {.hmm = &m->null.hmm, .R = &m->special_node.null.R},
        .alt = {
            .hmm = &m->alt.hmm,
            .S = &m->special_node.alt.S,
            .N = &m->special_node.alt.N,
            .B = &m->special_node.alt.B,
            .E = &m->special_node.alt.E,
            .J = &m->special_node.alt.J,
            .C = &m->special_node.alt.C,
            .T = &m->special_node.alt.T,
        }};
}

struct imm_amino const *pro_model_amino(struct dcp_pro_model const *m)
{
    return m->cfg.amino;
}

struct imm_nuclt const *pro_model_nuclt(struct dcp_pro_model const *m)
{
    return m->cfg.nuclt;
}

static int setup_distrs(struct imm_amino const *amino,
                        struct imm_nuclt const *nuclt,
                        imm_float const lprobs[IMM_AMINO_SIZE],
                        struct imm_nuclt_lprob *nucltp,
                        struct imm_codon_marg *codonm)
{
    int rc = IMM_SUCCESS;
    *nucltp = imm_nuclt_lprob(nuclt, uniform_nuclt_lprobs);
    struct imm_amino_lprob aminop = imm_amino_lprob(amino, lprobs);
    struct imm_codon_lprob codonp = setup_codonp(amino, &aminop, nucltp->nuclt);
    if ((rc = imm_codon_lprob_normalize(&codonp)))
        return rc;

    *nucltp = setup_nucltp(&codonp);
    *codonm = imm_codon_marg(&codonp);
    return rc;
}

static void add_special_node(struct dcp_pro_model *m)
{
    int rc = IMM_SUCCESS;
    struct special_node *n = &m->special_node;

    rc += imm_hmm_add_state(&m->null.hmm, imm_super(&n->null.R));
    rc += imm_hmm_set_start(&m->null.hmm, imm_super(&n->null.R), IMM_LPROB_ONE);

    rc += imm_hmm_add_state(&m->alt.hmm, imm_super(&n->alt.S));
    rc += imm_hmm_add_state(&m->alt.hmm, imm_super(&n->alt.N));
    rc += imm_hmm_add_state(&m->alt.hmm, imm_super(&n->alt.B));
    rc += imm_hmm_add_state(&m->alt.hmm, imm_super(&n->alt.E));
    rc += imm_hmm_add_state(&m->alt.hmm, imm_super(&n->alt.J));
    rc += imm_hmm_add_state(&m->alt.hmm, imm_super(&n->alt.C));
    rc += imm_hmm_add_state(&m->alt.hmm, imm_super(&n->alt.T));
    rc += imm_hmm_set_start(&m->alt.hmm, imm_super(&n->alt.S), IMM_LPROB_ONE);

    IMM_BUG(rc != IMM_SUCCESS);
}

static void calc_occupancy(struct dcp_pro_model *m, imm_float log_occ[static 1])
{
    struct dcp_pro_model_trans *trans = m->alt.trans;
    log_occ[0] = imm_lprob_add(trans->MI, trans->MM);
    for (unsigned i = 1; i < m->core_size; ++i)
    {
        ++trans;
        imm_float val0 = log_occ[i - 1] + imm_lprob_add(trans->MM, trans->MI);
        imm_float val1 = log1_p(log_occ[i - 1]) + trans->DM;
        log_occ[i] = imm_lprob_add(val0, val1);
    }

    imm_float logZ = imm_lprob_zero();
    unsigned n = m->core_size;
    for (unsigned i = 0; i < m->core_size; ++i)
    {
        logZ = imm_lprob_add(logZ, log_occ[i] + imm_log(n - i));
    }

    for (unsigned i = 0; i < m->core_size; ++i)
    {
        log_occ[i] -= logZ;
    }

    IMM_BUG(imm_lprob_is_nan(logZ));
}

static void init_special_trans(struct dcp_pro_model *m)
{
    int rc = IMM_SUCCESS;
    imm_float const o = IMM_LPROB_ONE;
    struct special_node *n = &m->special_node;

    struct imm_hmm *h = &m->null.hmm;
    rc += imm_hmm_set_trans(h, imm_super(&n->null.R), imm_super(&n->null.R), o);

    h = &m->alt.hmm;
    rc += imm_hmm_set_trans(h, imm_super(&n->alt.S), imm_super(&n->alt.B), o);
    rc += imm_hmm_set_trans(h, imm_super(&n->alt.S), imm_super(&n->alt.N), o);
    rc += imm_hmm_set_trans(h, imm_super(&n->alt.N), imm_super(&n->alt.N), o);
    rc += imm_hmm_set_trans(h, imm_super(&n->alt.N), imm_super(&n->alt.B), o);

    rc += imm_hmm_set_trans(h, imm_super(&n->alt.E), imm_super(&n->alt.T), o);
    rc += imm_hmm_set_trans(h, imm_super(&n->alt.E), imm_super(&n->alt.C), o);
    rc += imm_hmm_set_trans(h, imm_super(&n->alt.C), imm_super(&n->alt.C), o);
    rc += imm_hmm_set_trans(h, imm_super(&n->alt.C), imm_super(&n->alt.T), o);

    rc += imm_hmm_set_trans(h, imm_super(&n->alt.E), imm_super(&n->alt.B), o);
    rc += imm_hmm_set_trans(h, imm_super(&n->alt.E), imm_super(&n->alt.J), o);
    rc += imm_hmm_set_trans(h, imm_super(&n->alt.J), imm_super(&n->alt.J), o);
    rc += imm_hmm_set_trans(h, imm_super(&n->alt.J), imm_super(&n->alt.B), o);

    IMM_BUG(rc != IMM_SUCCESS);
}

static void init_delete(struct imm_mute_state *state, struct dcp_pro_model *m)
{
    unsigned id = DCP_PRO_MODEL_DELETE_ID | (m->alt.node_idx + 1);
    imm_mute_state_init(state, id, imm_super(m->cfg.nuclt));
}

static void init_insert(struct imm_frame_state *state, struct dcp_pro_model *m)
{
    imm_float e = m->cfg.epsilon;
    unsigned id = DCP_PRO_MODEL_INSERT_ID | (m->alt.node_idx + 1);
    struct imm_nuclt_lprob *nucltp = &m->alt.insert.nucltp;
    struct imm_codon_marg *codonm = &m->alt.insert.codonm;
    imm_frame_state_init(state, id, nucltp, codonm, e);
}

static void init_match(struct imm_frame_state *state, struct dcp_pro_model *m,
                       struct imm_nuclt_lprob *nucltp,
                       struct imm_codon_marg *codonm)
{
    imm_float e = m->cfg.epsilon;
    unsigned id = DCP_PRO_MODEL_MATCH_ID | (m->alt.node_idx + 1);
    imm_frame_state_init(state, id, nucltp, codonm, e);
}

static void new_special_node(struct dcp_pro_model *m)
{
    imm_float e = m->cfg.epsilon;
    struct imm_nuclt_lprob const *nucltp = &m->null.nucltp;
    struct imm_codon_marg const *codonm = &m->null.codonm;
    struct special_node *n = &m->special_node;
    struct imm_nuclt const *nuclt = m->cfg.nuclt;

    imm_frame_state_init(&n->null.R, DCP_PRO_MODEL_R_ID, nucltp, codonm, e);

    imm_mute_state_init(&n->alt.S, DCP_PRO_MODEL_S_ID, imm_super(nuclt));
    imm_frame_state_init(&n->alt.N, DCP_PRO_MODEL_N_ID, nucltp, codonm, e);
    imm_mute_state_init(&n->alt.B, DCP_PRO_MODEL_B_ID, imm_super(nuclt));
    imm_mute_state_init(&n->alt.E, DCP_PRO_MODEL_E_ID, imm_super(nuclt));
    imm_frame_state_init(&n->alt.J, DCP_PRO_MODEL_J_ID, nucltp, codonm, e);
    imm_frame_state_init(&n->alt.C, DCP_PRO_MODEL_C_ID, nucltp, codonm, e);
    imm_mute_state_init(&n->alt.T, DCP_PRO_MODEL_T_ID, imm_super(nuclt));
}

static struct imm_codon_lprob setup_codonp(struct imm_amino const *amino,
                                           struct imm_amino_lprob const *aminop,
                                           struct imm_nuclt const *nuclt)
{
    /* FIXME: We don't need 255 positions*/
    unsigned count[] = FILL(255, 0);

    for (unsigned i = 0; i < imm_gc_size(); ++i)
        count[(unsigned)imm_gc_aa(1, i)] += 1;

    struct imm_abc const *abc = imm_super(amino);
    /* TODO: We don't need 255 positions*/
    imm_float lprobs[] = FILL(255, IMM_LPROB_ZERO);
    for (unsigned i = 0; i < imm_abc_size(abc); ++i)
    {
        char aa = imm_abc_symbols(abc)[i];
        imm_float norm = imm_log((imm_float)count[(unsigned)aa]);
        lprobs[(unsigned)aa] = imm_amino_lprob_get(aminop, aa) - norm;
    }

    struct imm_codon_lprob codonp = imm_codon_lprob(nuclt);
    for (unsigned i = 0; i < imm_gc_size(); ++i)
    {
        char aa = imm_gc_aa(1, i);
        imm_codon_lprob_set(&codonp, imm_gc_codon(1, i), lprobs[(unsigned)aa]);
    }
    return codonp;
}

static void setup_entry_trans(struct dcp_pro_model *m)
{
    int rc = IMM_SUCCESS;
    if (m->cfg.edist == DCP_ENTRY_DIST_UNIFORM)
    {
        imm_float M = (imm_float)m->core_size;
        imm_float cost = imm_log(2.0 / (M * (M + 1))) * M;

        struct imm_state *B = imm_super(&m->special_node.alt.B);
        for (unsigned i = 0; i < m->core_size; ++i)
        {
            struct node *node = m->alt.nodes + i;
            rc += imm_hmm_set_trans(&m->alt.hmm, B, imm_super(&node->M), cost);
        }
    }
    else
    {
        IMM_BUG(m->cfg.edist != DCP_ENTRY_DIST_OCCUPANCY);
        imm_float *locc = xmalloc((m->core_size) * sizeof(*locc));
        calc_occupancy(m, locc);
        struct imm_state *B = imm_super(&m->special_node.alt.B);
        for (unsigned i = 0; i < m->core_size; ++i)
        {
            struct node *node = m->alt.nodes + i;
            rc +=
                imm_hmm_set_trans(&m->alt.hmm, B, imm_super(&node->M), locc[i]);
        }
        free(locc);
    }
    IMM_BUG(rc != IMM_SUCCESS);
}

static void setup_exit_trans(struct dcp_pro_model *m)
{
    int rc = IMM_SUCCESS;
    struct imm_state *E = imm_super(&m->special_node.alt.E);

    for (unsigned i = 0; i < m->core_size; ++i)
    {
        struct node *node = m->alt.nodes + i;
        rc +=
            imm_hmm_set_trans(&m->alt.hmm, imm_super(&node->M), E, imm_log(1));
    }
    for (unsigned i = 1; i < m->core_size; ++i)
    {
        struct node *node = m->alt.nodes + i;
        rc +=
            imm_hmm_set_trans(&m->alt.hmm, imm_super(&node->D), E, imm_log(1));
    }
    IMM_BUG(rc != IMM_SUCCESS);
}

static struct imm_nuclt_lprob setup_nucltp(struct imm_codon_lprob const *codonp)
{
    imm_float lprobs[] = FILL(IMM_NUCLT_SIZE, IMM_LPROB_ZERO);

    imm_float const norm = imm_log((imm_float)3);
    for (unsigned i = 0; i < imm_gc_size(); ++i)
    {
        struct imm_codon codon = imm_gc_codon(1, i);
        imm_float lprob = imm_codon_lprob_get(codonp, codon);
        lprobs[codon.a] = imm_lprob_add(lprobs[codon.a], lprob - norm);
        lprobs[codon.b] = imm_lprob_add(lprobs[codon.b], lprob - norm);
        lprobs[codon.c] = imm_lprob_add(lprobs[codon.c], lprob - norm);
    }
    return imm_nuclt_lprob(codonp->nuclt, lprobs);
}

static void setup_trans(struct dcp_pro_model *m)
{
    struct imm_hmm *h = &m->alt.hmm;
    struct dcp_pro_model_trans *trans = m->alt.trans;

    struct imm_state *B = imm_super(&m->special_node.alt.B);
    struct imm_state *M1 = imm_super(&m->alt.nodes[0].M);
    int rc = imm_hmm_set_trans(h, B, M1, trans[0].MM);

    for (unsigned i = 0; i + 1 < m->core_size; ++i)
    {
        struct node prev = m->alt.nodes[i];
        struct node next = m->alt.nodes[i + 1];
        unsigned j = i + 1;
        struct dcp_pro_model_trans t = trans[j];
        rc +=
            imm_hmm_set_trans(h, imm_super(&prev.M), imm_super(&prev.I), t.MI);
        rc +=
            imm_hmm_set_trans(h, imm_super(&prev.I), imm_super(&prev.I), t.II);
        rc +=
            imm_hmm_set_trans(h, imm_super(&prev.M), imm_super(&next.M), t.MM);
        rc +=
            imm_hmm_set_trans(h, imm_super(&prev.I), imm_super(&next.M), t.IM);
        rc +=
            imm_hmm_set_trans(h, imm_super(&prev.M), imm_super(&next.D), t.MD);
        rc +=
            imm_hmm_set_trans(h, imm_super(&prev.D), imm_super(&next.D), t.DD);
        rc +=
            imm_hmm_set_trans(h, imm_super(&prev.D), imm_super(&next.M), t.DM);
    }

    unsigned n = m->core_size;
    struct imm_state *Mm = imm_super(&m->alt.nodes[n - 1].M);
    struct imm_state *E = imm_super(&m->special_node.alt.E);
    rc += imm_hmm_set_trans(h, Mm, E, trans[n].MM);

    IMM_BUG(rc != IMM_SUCCESS);

    setup_entry_trans(m);
    setup_exit_trans(m);
    init_special_trans(m);
}
