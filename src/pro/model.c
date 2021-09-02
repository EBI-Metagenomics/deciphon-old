#include "dcp/pro/model.h"
#include "dcp/entry_dist.h"
#include "dcp/pro/node.h"
#include "error.h"
#include "pro/model.h"
#include <assert.h>
#include <limits.h>
#include <stdlib.h>

struct nuclt_dist
{
    struct imm_nuclt_lprob *nucltp;
    struct imm_codon_marg *codonm;
};

#define LOGN2 (imm_float)(-1.3862943611198906) /* log(1./4.) */

static imm_float const uniform_lprobs[IMM_NUCLT_SIZE] = {LOGN2, LOGN2, LOGN2,
                                                         LOGN2};

/* Compute log(1 - p) given log(p). */
static inline imm_float log1_p(imm_float logp) { return log1p(-exp(logp)); }

static void add_xnodes(struct dcp_pro_model *);
static void init_xnodes(struct dcp_pro_model *);

static void calculate_occupancy(struct dcp_pro_model *);

static bool have_called_setup(struct dcp_pro_model *);
static bool have_finished_add(struct dcp_pro_model const *);

static void init_delete(struct imm_mute_state *, struct dcp_pro_model *);
static void init_insert(struct imm_frame_state *, struct dcp_pro_model *);
static void init_match(struct imm_frame_state *, struct dcp_pro_model *,
                       struct nuclt_dist *);

static void init_null_xtrans(struct imm_hmm *, struct dcp_pro_xnode_null *);
static void init_alt_xtrans(struct imm_hmm *, struct dcp_pro_xnode_alt *);

static struct imm_nuclt_lprob nuclt_lprob(struct imm_codon_lprob const *);
static struct imm_codon_lprob codon_lprob(struct imm_amino const *,
                                          struct imm_amino_lprob const *,
                                          struct imm_nuclt const *);

static void setup_nuclt_dist(struct nuclt_dist *, struct imm_amino const *,
                             struct imm_nuclt const *,
                             imm_float const[IMM_AMINO_SIZE]);

static void setup_entry_trans(struct dcp_pro_model *);
static void setup_exit_trans(struct dcp_pro_model *);
static void setup_transitions(struct dcp_pro_model *);

enum dcp_rc dcp_pro_model_add_node(struct dcp_pro_model *m,
                                   imm_float const lprobs[IMM_AMINO_SIZE])
{
    if (!have_called_setup(m))
        return error(DCP_RUNTIMEERROR, "Must call dcp_pro_model_setup first.");

    if (m->alt.node_idx == m->core_size)
        return error(DCP_RUNTIMEERROR, "Reached limit of nodes.");

    imm_float lodds[IMM_AMINO_SIZE];
    for (unsigned i = 0; i < IMM_AMINO_SIZE; ++i)
        lodds[i] = lprobs[i] - m->null.lprobs[i];

    struct dcp_pro_node *n = m->alt.nodes + m->alt.node_idx;

    struct nuclt_dist dist = {&n->match.nucltp, &n->match.codonm};
    setup_nuclt_dist(&dist, m->cfg.amino, m->cfg.nuclt, lodds);

    init_match(&n->M, m, &dist);
    if (imm_hmm_add_state(&m->alt.hmm, imm_super(&n->M)))
        return DCP_RUNTIMEERROR;

    init_insert(&n->I, m);
    if (imm_hmm_add_state(&m->alt.hmm, imm_super(&n->I)))
        return DCP_RUNTIMEERROR;

    init_delete(&n->D, m);
    if (imm_hmm_add_state(&m->alt.hmm, imm_super(&n->D)))
        return DCP_RUNTIMEERROR;

    m->alt.node_idx++;

    if (have_finished_add(m)) setup_transitions(m);

    return DCP_SUCCESS;
}

enum dcp_rc dcp_pro_model_add_trans(struct dcp_pro_model *m,
                                    struct dcp_pro_trans trans)
{
    if (!have_called_setup(m))
        return error(DCP_RUNTIMEERROR, "Must call dcp_pro_model_setup first.");

    if (m->alt.trans_idx == m->core_size + 1)
        return error(DCP_RUNTIMEERROR, "Reached limit of transitions.");

    m->alt.trans[m->alt.trans_idx++] = trans;
    if (have_finished_add(m)) setup_transitions(m);
    return DCP_SUCCESS;
}

void dcp_pro_model_del(struct dcp_pro_model const *model)
{
    free(model->alt.nodes);
    free(model->alt.locc);
    free(model->alt.trans);
}

void dcp_pro_model_init(struct dcp_pro_model *m, struct dcp_pro_cfg cfg,
                        imm_float const null_lprobs[IMM_AMINO_SIZE])

{
    m->cfg = cfg;
    m->core_size = 0;

    memcpy(m->null.lprobs, null_lprobs, sizeof(*null_lprobs) * IMM_AMINO_SIZE);

    imm_hmm_init(&m->null.hmm, imm_super(m->cfg.nuclt));

    struct nuclt_dist null_dist = {&m->null.nucltp, &m->null.codonm};
    setup_nuclt_dist(&null_dist, cfg.amino, cfg.nuclt, null_lprobs);

    imm_hmm_init(&m->alt.hmm, imm_super(m->cfg.nuclt));

    struct nuclt_dist alt_dist = {&m->alt.insert.nucltp, &m->alt.insert.codonm};
    imm_float const lodds[IMM_AMINO_SIZE] = {0.0f};
    setup_nuclt_dist(&alt_dist, cfg.amino, cfg.nuclt, lodds);

    init_xnodes(m);

    m->alt.node_idx = UINT_MAX;
    m->alt.nodes = NULL;
    m->alt.locc = NULL;
    m->alt.trans_idx = UINT_MAX;
    m->alt.trans = NULL;
    dcp_pro_xtrans_init(&m->xtrans);
}

enum dcp_rc dcp_pro_model_setup(struct dcp_pro_model *m, unsigned core_size)
{
    if (core_size == 0)
        return error(DCP_ILLEGALARG, "`core_size` cannot be zero.");

    m->core_size = core_size;
    unsigned n = m->core_size;
    m->alt.node_idx = 0;
    m->alt.nodes = realloc(m->alt.nodes, n * sizeof(*m->alt.nodes));
    if (m->cfg.edist == DCP_ENTRY_DIST_OCCUPANCY)
        m->alt.locc = realloc(m->alt.locc, n * sizeof(*m->alt.locc));
    m->alt.trans_idx = 0;
    m->alt.trans = realloc(m->alt.trans, (n + 1) * sizeof(*m->alt.trans));
    imm_hmm_reset(&m->alt.hmm);
    imm_hmm_reset(&m->null.hmm);
    add_xnodes(m);
    return DCP_SUCCESS;
}

void dcp_pro_model_write_dot(struct dcp_pro_model const *m, FILE *restrict fp)
{
    imm_hmm_write_dot(&m->alt.hmm, fp, pro_model_state_name);
}

struct imm_amino const *pro_model_amino(struct dcp_pro_model const *m)
{
    return m->cfg.amino;
}

struct imm_nuclt const *pro_model_nuclt(struct dcp_pro_model const *m)
{
    return m->cfg.nuclt;
}

void pro_model_state_name(unsigned id, char name[IMM_STATE_NAME_SIZE])
{
    unsigned msb = id & (3U << (DCP_PROFILE_BITS_ID - 2));
    if (msb == DCP_PRO_ID_EXT)
    {
        if (id == DCP_PRO_ID_R)
            name[0] = 'R';
        else if (id == DCP_PRO_ID_S)
            name[0] = 'S';
        else if (id == DCP_PRO_ID_N)
            name[0] = 'N';
        else if (id == DCP_PRO_ID_B)
            name[0] = 'B';
        else if (id == DCP_PRO_ID_E)
            name[0] = 'E';
        else if (id == DCP_PRO_ID_J)
            name[0] = 'J';
        else if (id == DCP_PRO_ID_C)
            name[0] = 'C';
        else if (id == DCP_PRO_ID_T)
            name[0] = 'T';
        name[1] = '\0';
    }
    else
    {
        if (msb == DCP_PRO_ID_MATCH)
            name[0] = 'M';
        else if (msb == DCP_PRO_ID_INSERT)
            name[0] = 'I';
        else if (msb == DCP_PRO_ID_DELETE)
            name[0] = 'D';
        unsigned idx = id & (0xFFFF >> 2);
        snprintf(name + 1, 7, "%d", idx);
    }
}

struct pro_model_summary pro_model_summary(struct dcp_pro_model const *m)
{
    assert(have_finished_add(m));
    return (struct pro_model_summary){
        .null = {.hmm = &m->null.hmm, .R = &m->xnode.null.R},
        .alt = {
            .hmm = &m->alt.hmm,
            .S = &m->xnode.alt.S,
            .N = &m->xnode.alt.N,
            .B = &m->xnode.alt.B,
            .E = &m->xnode.alt.E,
            .J = &m->xnode.alt.J,
            .C = &m->xnode.alt.C,
            .T = &m->xnode.alt.T,
        }};
}

static void add_xnodes(struct dcp_pro_model *m)
{
    enum dcp_rc rc = DCP_SUCCESS;
    struct dcp_pro_xnode *n = &m->xnode;

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

    assert(rc == DCP_SUCCESS);
}

static void init_xnodes(struct dcp_pro_model *m)
{
    imm_float e = m->cfg.epsilon;
    struct imm_nuclt_lprob const *nucltp = &m->null.nucltp;
    struct imm_codon_marg const *codonm = &m->null.codonm;
    struct dcp_pro_xnode *n = &m->xnode;
    struct imm_nuclt const *nuclt = m->cfg.nuclt;

    imm_frame_state_init(&n->null.R, DCP_PRO_ID_R, nucltp, codonm, e);

    imm_mute_state_init(&n->alt.S, DCP_PRO_ID_S, imm_super(nuclt));
    imm_frame_state_init(&n->alt.N, DCP_PRO_ID_N, nucltp, codonm, e);
    imm_mute_state_init(&n->alt.B, DCP_PRO_ID_B, imm_super(nuclt));
    imm_mute_state_init(&n->alt.E, DCP_PRO_ID_E, imm_super(nuclt));
    imm_frame_state_init(&n->alt.J, DCP_PRO_ID_J, nucltp, codonm, e);
    imm_frame_state_init(&n->alt.C, DCP_PRO_ID_C, nucltp, codonm, e);
    imm_mute_state_init(&n->alt.T, DCP_PRO_ID_T, imm_super(nuclt));
}

static void calculate_occupancy(struct dcp_pro_model *m)
{
    struct dcp_pro_trans *trans = m->alt.trans;
    m->alt.locc[0] = imm_lprob_add(trans->MI, trans->MM);
    for (unsigned i = 1; i < m->core_size; ++i)
    {
        ++trans;
        imm_float v0 = m->alt.locc[i - 1] + imm_lprob_add(trans->MM, trans->MI);
        imm_float v1 = log1_p(m->alt.locc[i - 1]) + trans->DM;
        m->alt.locc[i] = imm_lprob_add(v0, v1);
    }

    imm_float logZ = imm_lprob_zero();
    unsigned n = m->core_size;
    for (unsigned i = 0; i < m->core_size; ++i)
    {
        logZ = imm_lprob_add(logZ, m->alt.locc[i] + imm_log(n - i));
    }

    for (unsigned i = 0; i < m->core_size; ++i)
    {
        m->alt.locc[i] -= logZ;
    }

    assert(!imm_lprob_is_nan(logZ));
}

static bool have_called_setup(struct dcp_pro_model *m)
{
    return m->core_size > 0;
}

static bool have_finished_add(struct dcp_pro_model const *m)
{
    unsigned core_size = m->core_size;
    return m->alt.node_idx == core_size && m->alt.trans_idx == (core_size + 1);
}

static void init_delete(struct imm_mute_state *state, struct dcp_pro_model *m)
{
    unsigned id = DCP_PRO_ID_DELETE | (m->alt.node_idx + 1);
    imm_mute_state_init(state, id, imm_super(m->cfg.nuclt));
}

static void init_insert(struct imm_frame_state *state, struct dcp_pro_model *m)
{
    imm_float e = m->cfg.epsilon;
    unsigned id = DCP_PRO_ID_INSERT | (m->alt.node_idx + 1);
    struct imm_nuclt_lprob *nucltp = &m->alt.insert.nucltp;
    struct imm_codon_marg *codonm = &m->alt.insert.codonm;
    imm_frame_state_init(state, id, nucltp, codonm, e);
}

static void init_match(struct imm_frame_state *state, struct dcp_pro_model *m,
                       struct nuclt_dist *d)
{
    imm_float e = m->cfg.epsilon;
    unsigned id = DCP_PRO_ID_MATCH | (m->alt.node_idx + 1);
    imm_frame_state_init(state, id, d->nucltp, d->codonm, e);
}

static void init_null_xtrans(struct imm_hmm *hmm,
                             struct dcp_pro_xnode_null *node)
{
    enum dcp_rc rc = DCP_SUCCESS;
    imm_float const o = IMM_LPROB_ONE;
    rc += imm_hmm_set_trans(hmm, imm_super(&node->R), imm_super(&node->R), o);
    assert(rc == DCP_SUCCESS);
}

static void init_alt_xtrans(struct imm_hmm *hmm, struct dcp_pro_xnode_alt *node)
{
    enum dcp_rc rc = DCP_SUCCESS;
    imm_float const o = IMM_LPROB_ONE;
    rc += imm_hmm_set_trans(hmm, imm_super(&node->S), imm_super(&node->B), o);
    rc += imm_hmm_set_trans(hmm, imm_super(&node->S), imm_super(&node->N), o);
    rc += imm_hmm_set_trans(hmm, imm_super(&node->N), imm_super(&node->N), o);
    rc += imm_hmm_set_trans(hmm, imm_super(&node->N), imm_super(&node->B), o);

    rc += imm_hmm_set_trans(hmm, imm_super(&node->E), imm_super(&node->T), o);
    rc += imm_hmm_set_trans(hmm, imm_super(&node->E), imm_super(&node->C), o);
    rc += imm_hmm_set_trans(hmm, imm_super(&node->C), imm_super(&node->C), o);
    rc += imm_hmm_set_trans(hmm, imm_super(&node->C), imm_super(&node->T), o);

    rc += imm_hmm_set_trans(hmm, imm_super(&node->E), imm_super(&node->B), o);
    rc += imm_hmm_set_trans(hmm, imm_super(&node->E), imm_super(&node->J), o);
    rc += imm_hmm_set_trans(hmm, imm_super(&node->J), imm_super(&node->J), o);
    rc += imm_hmm_set_trans(hmm, imm_super(&node->J), imm_super(&node->B), o);

    assert(rc == DCP_SUCCESS);
}

static struct imm_nuclt_lprob nuclt_lprob(struct imm_codon_lprob const *codonp)
{
    imm_float lprobs[] = {[0 ... IMM_NUCLT_SIZE - 1] = IMM_LPROB_ZERO};

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

static struct imm_codon_lprob codon_lprob(struct imm_amino const *amino,
                                          struct imm_amino_lprob const *aminop,
                                          struct imm_nuclt const *nuclt)
{
    /* FIXME: We don't need 255 positions*/
    unsigned count[] = {[0 ... 254] = 0};

    for (unsigned i = 0; i < imm_gc_size(); ++i)
        count[(unsigned)imm_gc_aa(1, i)] += 1;

    struct imm_abc const *abc = imm_super(amino);
    /* TODO: We don't need 255 positions*/
    imm_float lprobs[] = {[0 ... 254] = IMM_LPROB_ZERO};
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

static void setup_nuclt_dist(struct nuclt_dist *dist,
                             struct imm_amino const *amino,
                             struct imm_nuclt const *nuclt,
                             imm_float const lprobs[IMM_AMINO_SIZE])
{
    *dist->nucltp = imm_nuclt_lprob(nuclt, uniform_lprobs);
    struct imm_amino_lprob aminop = imm_amino_lprob(amino, lprobs);
    struct imm_codon_lprob codonp =
        codon_lprob(amino, &aminop, dist->nucltp->nuclt);
    imm_codon_lprob_normalize(&codonp);

    *dist->nucltp = nuclt_lprob(&codonp);
    *dist->codonm = imm_codon_marg(&codonp);
}

static void setup_entry_trans(struct dcp_pro_model *m)
{
    enum dcp_rc rc = DCP_SUCCESS;
    if (m->cfg.edist == DCP_ENTRY_DIST_UNIFORM)
    {
        imm_float M = (imm_float)m->core_size;
        imm_float cost = imm_log(2.0 / (M * (M + 1))) * M;

        struct imm_state *B = imm_super(&m->xnode.alt.B);
        for (unsigned i = 0; i < m->core_size; ++i)
        {
            struct dcp_pro_node *node = m->alt.nodes + i;
            rc += imm_hmm_set_trans(&m->alt.hmm, B, imm_super(&node->M), cost);
        }
    }
    else
    {
        assert(m->cfg.edist == DCP_ENTRY_DIST_OCCUPANCY);
        calculate_occupancy(m);
        struct imm_state *B = imm_super(&m->xnode.alt.B);
        for (unsigned i = 0; i < m->core_size; ++i)
        {
            struct dcp_pro_node *node = m->alt.nodes + i;
            rc += imm_hmm_set_trans(&m->alt.hmm, B, imm_super(&node->M),
                                    m->alt.locc[i]);
        }
    }
    assert(rc == DCP_SUCCESS);
}

static void setup_exit_trans(struct dcp_pro_model *m)
{
    enum dcp_rc rc = DCP_SUCCESS;
    struct imm_state *E = imm_super(&m->xnode.alt.E);

    for (unsigned i = 0; i < m->core_size; ++i)
    {
        struct dcp_pro_node *node = m->alt.nodes + i;
        rc +=
            imm_hmm_set_trans(&m->alt.hmm, imm_super(&node->M), E, imm_log(1));
    }
    for (unsigned i = 1; i < m->core_size; ++i)
    {
        struct dcp_pro_node *node = m->alt.nodes + i;
        rc +=
            imm_hmm_set_trans(&m->alt.hmm, imm_super(&node->D), E, imm_log(1));
    }
    assert(rc == DCP_SUCCESS);
}

static void setup_transitions(struct dcp_pro_model *m)
{
    struct imm_hmm *h = &m->alt.hmm;
    struct dcp_pro_trans *trans = m->alt.trans;

    struct imm_state *B = imm_super(&m->xnode.alt.B);
    struct imm_state *M1 = imm_super(&m->alt.nodes[0].M);
    int rc = (int)imm_hmm_set_trans(h, B, M1, trans[0].MM);

    for (unsigned i = 0; i + 1 < m->core_size; ++i)
    {
        struct dcp_pro_node *pr = m->alt.nodes + i;
        struct dcp_pro_node *nx = m->alt.nodes + i + 1;
        unsigned j = i + 1;
        struct dcp_pro_trans t = trans[j];
        rc += imm_hmm_set_trans(h, imm_super(&pr->M), imm_super(&pr->I), t.MI);
        rc += imm_hmm_set_trans(h, imm_super(&pr->I), imm_super(&pr->I), t.II);
        rc += imm_hmm_set_trans(h, imm_super(&pr->M), imm_super(&nx->M), t.MM);
        rc += imm_hmm_set_trans(h, imm_super(&pr->I), imm_super(&nx->M), t.IM);
        rc += imm_hmm_set_trans(h, imm_super(&pr->M), imm_super(&nx->D), t.MD);
        rc += imm_hmm_set_trans(h, imm_super(&pr->D), imm_super(&nx->D), t.DD);
        rc += imm_hmm_set_trans(h, imm_super(&pr->D), imm_super(&nx->M), t.DM);
    }

    unsigned n = m->core_size;
    struct imm_state *Mm = imm_super(&m->alt.nodes[n - 1].M);
    struct imm_state *E = imm_super(&m->xnode.alt.E);
    rc += imm_hmm_set_trans(h, Mm, E, trans[n].MM);

    assert(!rc);

    setup_entry_trans(m);
    setup_exit_trans(m);
    init_null_xtrans(&m->null.hmm, &m->xnode.null);
    init_alt_xtrans(&m->alt.hmm, &m->xnode.alt);
}
