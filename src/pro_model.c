#include "dcp/entry_distr.h"
#include "dcp/pro_model.h"
#include "imm/imm.h"
#include "pro_model.h"
#include "support.h"

struct node
{
    struct imm_frame_state *M;
    struct imm_frame_state *I;
    struct imm_mute_state *D;
    struct imm_nuclt_lprob nucltp;
    struct imm_codon_marg codonm;
};

struct special_node
{
    struct
    {
        struct imm_frame_state *R;
    } null;

    struct
    {
        struct imm_mute_state *S;
        struct imm_frame_state *N;
        struct imm_mute_state *B;
        struct imm_mute_state *E;
        struct imm_frame_state *J;
        struct imm_frame_state *C;
        struct imm_mute_state *T;
    } alt;
};

struct dcp_pro_model
{
    struct imm_amino const *amino;
    struct imm_nuclt const *nuclt;
    unsigned core_size;
    imm_float epsilon;
    enum dcp_entry_distr edist;
    struct special_node special;
    struct pro_model_special_trans special_trans;
    bool ready;

    struct
    {
        imm_float lprobs[IMM_AMINO_SIZE];
        struct imm_nuclt_lprob nucltp;
        struct imm_codon_marg codonm;
        struct imm_hmm *hmm;
    } null;

    struct
    {
        struct node *nodes;
        struct dcp_pro_model_trans *trans;
        struct imm_hmm *hmm;
        unsigned idx;

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

static int setup_distrs(struct imm_amino const *amino,
                        struct imm_nuclt const *nuclt,
                        imm_float const lprobs[IMM_AMINO_SIZE],
                        struct imm_nuclt_lprob *nucltp,
                        struct imm_codon_marg *codonm);

static struct imm_codon_lprob setup_codonp(struct imm_amino const *amino,
                                           struct imm_amino_lprob const *aminop,
                                           struct imm_nuclt const *nuclt);

static struct imm_nuclt_lprob
setup_nucltp(struct imm_codon_lprob const *codonp);

static int setup_transitions(struct dcp_pro_model *model);

static void setup_entry_transitions(struct dcp_pro_model *model);

static void set_exit_transitions(struct dcp_pro_model *model);

static void calculate_occupancy(struct dcp_pro_model *m,
                                imm_float log_occ[static 1]);

static void create_special_node(struct dcp_pro_model *m);

static void add_special_node(struct dcp_pro_model *m);

static void init_special_trans(struct dcp_pro_model *m);

struct dcp_pro_model *
dcp_pro_model_new(struct imm_amino const *amino, struct imm_nuclt const *nuclt,
                  imm_float const null_lprobs[IMM_AMINO_SIZE],
                  imm_float const null_lodds[IMM_AMINO_SIZE], imm_float epsilon,
                  unsigned core_size, enum dcp_entry_distr edistr)

{
    struct dcp_pro_model *m = xmalloc(sizeof(*m));
    m->amino = amino;
    m->nuclt = nuclt;
    m->core_size = core_size;
    m->epsilon = epsilon;
    m->edist = edistr;
    m->ready = false;
    m->alt.hmm = NULL;
    m->alt.idx = 0;
    m->null.hmm = NULL;

    memcpy(m->null.lprobs, null_lprobs, sizeof(*null_lprobs) * IMM_AMINO_SIZE);

    if (!(m->null.hmm = imm_hmm_new(imm_super(m->nuclt))))
        goto cleanup;

    if (setup_distrs(amino, nuclt, null_lprobs, &m->null.nucltp,
                     &m->null.codonm))
        goto cleanup;

    if (!(m->alt.hmm = imm_hmm_new(imm_super(m->nuclt))))
        goto cleanup;

    if (setup_distrs(amino, nuclt, null_lodds, &m->alt.insert.nucltp,
                     &m->alt.insert.codonm))
        goto cleanup;

    create_special_node(m);
    add_special_node(m);

    m->alt.nodes = xcalloc(core_size, sizeof(*m->alt.nodes));
    m->alt.trans = xcalloc(core_size + 1, sizeof(*m->alt.trans));
    m->special_trans = pro_model_special_trans_init();

    return m;

cleanup:

    imm_del(m->special.null.R);
    imm_del(m->special.alt.S);
    imm_del(m->special.alt.N);
    imm_del(m->special.alt.B);
    imm_del(m->special.alt.E);
    imm_del(m->special.alt.J);
    imm_del(m->special.alt.C);
    imm_del(m->special.alt.T);
    imm_del(m->null.hmm);
    imm_del(m->alt.hmm);
    free(m->alt.nodes);
    free(m->alt.trans);
    free(m);
    return NULL;
}

int dcp_pro_model_add_trans(struct dcp_pro_model *m,
                            struct dcp_pro_model_trans trans)
{
    int rc = IMM_SUCCESS;
    m->alt.trans[m->alt.idx++] = trans;
    if (m->alt.idx == m->core_size + 1)
    {
        if ((rc = setup_transitions(m)))
            return rc;

        init_special_trans(m);
        m->ready = true;
    }
    return rc;
}

static struct imm_frame_state *new_match(struct dcp_pro_model *m,
                                         struct imm_nuclt_lprob *nucltp,
                                         struct imm_codon_marg *codonm)
{
    imm_float e = m->epsilon;
    unsigned id = DCP_PRO_MODEL_MATCH_ID | (m->alt.idx + 1);
    struct imm_frame_state *frame = imm_frame_state_new(id, nucltp, codonm, e);
    return frame;
}

static struct imm_frame_state *new_insert(struct dcp_pro_model *m)
{
    imm_float e = m->epsilon;
    unsigned id = DCP_PRO_MODEL_INSERT_ID | (m->alt.idx + 1);
    struct imm_nuclt_lprob *nucltp = &m->alt.insert.nucltp;
    struct imm_codon_marg *codonm = &m->alt.insert.codonm;
    struct imm_frame_state *frame = imm_frame_state_new(id, nucltp, codonm, e);
    return frame;
}

static struct imm_mute_state *new_delete(struct dcp_pro_model *m)
{
    unsigned id = DCP_PRO_MODEL_DELETE_ID | (m->alt.idx + 1);
    return imm_mute_state_new(id, imm_super(m->nuclt));
}

int dcp_pro_model_add_node(struct dcp_pro_model *m,
                           imm_float const lprobs[IMM_AMINO_SIZE])
{
    int rc = IMM_SUCCESS;

    imm_float lodds[IMM_AMINO_SIZE];
    for (unsigned i = 0; i < IMM_AMINO_SIZE; ++i)
        lodds[i] = lprobs[i] - m->null.lprobs[i];

    struct node *n = m->alt.nodes + m->alt.idx;

    if ((rc = setup_distrs(m->amino, m->nuclt, lodds, &n->nucltp, &n->codonm)))
        return rc;

    struct imm_frame_state *M = new_match(m, &n->nucltp, &n->codonm);

    if ((rc = imm_hmm_add_state(m->alt.hmm, imm_super(M))))
        return rc;

    struct imm_frame_state *ins = new_insert(m);
    if ((rc = imm_hmm_add_state(m->alt.hmm, imm_super(ins))))
        return rc;

    struct imm_mute_state *del = new_delete(m);
    if ((rc = imm_hmm_add_state(m->alt.hmm, imm_super(del))))
        return rc;

    n->M = M;
    n->I = ins;
    n->D = del;
    m->alt.idx++;

    if (m->alt.idx == m->core_size)
        m->alt.idx = 0;

    return rc;
}

void dcp_pro_model_del(struct dcp_pro_model const *model)
{
    if (model)
    {
        imm_del(model->special.null.R);
        imm_del(model->special.alt.S);
        imm_del(model->special.alt.N);
        imm_del(model->special.alt.B);
        imm_del(model->special.alt.E);
        imm_del(model->special.alt.J);
        imm_del(model->special.alt.C);
        imm_del(model->special.alt.T);
        imm_del(model->null.hmm);
        imm_del(model->alt.hmm);

        for (unsigned i = 0; i < model->core_size; ++i)
        {
            imm_del(model->alt.nodes[i].M);
            imm_del(model->alt.nodes[i].I);
            imm_del(model->alt.nodes[i].D);
        }

        free(model->alt.nodes);
        free(model->alt.trans);
        free((void *)model);
    }
}

struct pro_model_summary pro_model_summary(struct dcp_pro_model const *m)
{
    IMM_BUG(!m->ready);
    return (struct pro_model_summary){
        .null = {.hmm = m->null.hmm, .R = m->special.null.R},
        .alt = {
            .hmm = m->alt.hmm,
            .S = m->special.alt.S,
            .N = m->special.alt.N,
            .B = m->special.alt.B,
            .E = m->special.alt.E,
            .J = m->special.alt.J,
            .C = m->special.alt.C,
            .T = m->special.alt.T,
        }};
}

struct imm_amino const *pro_model_amino(struct dcp_pro_model const *m)
{
    return m->amino;
}

struct imm_nuclt const *pro_model_nuclt(struct dcp_pro_model const *m)
{
    return m->nuclt;
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

static int setup_transitions(struct dcp_pro_model *m)
{
    struct imm_hmm *hmm = m->alt.hmm;
    struct dcp_pro_model_trans *trans = m->alt.trans;

    struct imm_state *B = imm_super(m->special.alt.B);
    struct imm_state *M1 = imm_super(m->alt.nodes[0].M);
    imm_hmm_set_trans(hmm, B, M1, trans[0].MM);

    for (unsigned i = 0; i + 1 < m->core_size; ++i)
    {
        struct node prev = m->alt.nodes[i];
        struct node next = m->alt.nodes[i + 1];
        unsigned j = i + 1;
        imm_hmm_set_trans(hmm, imm_super(prev.M), imm_super(prev.I),
                          trans[j].MI);
        imm_hmm_set_trans(hmm, imm_super(prev.I), imm_super(prev.I),
                          trans[j].II);
        imm_hmm_set_trans(hmm, imm_super(prev.M), imm_super(next.M),
                          trans[j].MM);
        imm_hmm_set_trans(hmm, imm_super(prev.I), imm_super(next.M),
                          trans[j].IM);
        imm_hmm_set_trans(hmm, imm_super(prev.M), imm_super(next.D),
                          trans[j].MD);
        imm_hmm_set_trans(hmm, imm_super(prev.D), imm_super(next.D),
                          trans[j].DD);
        imm_hmm_set_trans(hmm, imm_super(prev.D), imm_super(next.M),
                          trans[j].DM);
    }

    unsigned n = m->core_size;
    struct imm_state *Mm = imm_super(m->alt.nodes[n - 1].M);
    imm_hmm_set_trans(hmm, Mm, imm_super(m->special.alt.E), trans[n].MM);

    setup_entry_transitions(m);
    set_exit_transitions(m);
    return IMM_SUCCESS;
}

static void setup_entry_transitions(struct dcp_pro_model *m)
{
    if (m->edist == DCP_ENTRY_DISTR_UNIFORM)
    {
        imm_float M = (imm_float)m->core_size;
        imm_float cost = imm_log(2.0 / (M * (M + 1))) * M;

        struct imm_state *B = imm_super(m->special.alt.B);
        for (unsigned i = 0; i < m->core_size; ++i)
        {
            struct node *node = m->alt.nodes + i;
            imm_hmm_set_trans(m->alt.hmm, B, imm_super(node->M), cost);
        }
    }
    else
    {
        IMM_BUG(m->edist != DCP_ENTRY_DISTR_OCCUPANCY);
        imm_float *locc = xmalloc((m->core_size) * sizeof(*locc));
        calculate_occupancy(m, locc);
        struct imm_state *B = imm_super(m->special.alt.B);
        for (unsigned i = 0; i < m->core_size; ++i)
        {
            struct node *node = m->alt.nodes + i;
            imm_hmm_set_trans(m->alt.hmm, B, imm_super(node->M), locc[i]);
        }
        free(locc);
    }
}

static void set_exit_transitions(struct dcp_pro_model *m)
{
    struct imm_state *E = imm_super(m->special.alt.E);

    for (unsigned i = 0; i < m->core_size; ++i)
    {
        struct node *node = m->alt.nodes + i;
        imm_hmm_set_trans(m->alt.hmm, imm_super(node->M), E, imm_log(1));
    }
    for (unsigned i = 1; i < m->core_size; ++i)
    {
        struct node *node = m->alt.nodes + i;
        imm_hmm_set_trans(m->alt.hmm, imm_super(node->D), E, imm_log(1));
    }
}

static inline imm_float log1_p(imm_float logp)
{
    /* Compute log(1 - p) given log(p). */
    return log1p(-exp(logp));
}

static void calculate_occupancy(struct dcp_pro_model *m,
                                imm_float log_occ[static 1])
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
}

static void create_special_node(struct dcp_pro_model *m)
{
    imm_float e = m->epsilon;
    struct imm_nuclt_lprob const *nucltp = &m->null.nucltp;
    struct imm_codon_marg const *codonm = &m->null.codonm;
    struct special_node *n = &m->special;

    n->null.R = imm_frame_state_new(DCP_PRO_MODEL_R_ID, nucltp, codonm, e);

    n->alt.S = imm_mute_state_new(DCP_PRO_MODEL_S_ID, imm_super(m->nuclt));
    n->alt.N = imm_frame_state_new(DCP_PRO_MODEL_N_ID, nucltp, codonm, e);
    n->alt.B = imm_mute_state_new(DCP_PRO_MODEL_B_ID, imm_super(m->nuclt));
    n->alt.E = imm_mute_state_new(DCP_PRO_MODEL_E_ID, imm_super(m->nuclt));
    n->alt.J = imm_frame_state_new(DCP_PRO_MODEL_J_ID, nucltp, codonm, e);
    n->alt.C = imm_frame_state_new(DCP_PRO_MODEL_C_ID, nucltp, codonm, e);
    n->alt.T = imm_mute_state_new(DCP_PRO_MODEL_T_ID, imm_super(m->nuclt));
}

static void add_special_node(struct dcp_pro_model *m)
{
    imm_hmm_add_state(m->null.hmm, imm_super(m->special.null.R));
    imm_hmm_set_start(m->null.hmm, imm_super(m->special.null.R), imm_log(1));

    imm_hmm_add_state(m->alt.hmm, imm_super(m->special.alt.S));
    imm_hmm_add_state(m->alt.hmm, imm_super(m->special.alt.N));
    imm_hmm_add_state(m->alt.hmm, imm_super(m->special.alt.B));
    imm_hmm_add_state(m->alt.hmm, imm_super(m->special.alt.E));
    imm_hmm_add_state(m->alt.hmm, imm_super(m->special.alt.J));
    imm_hmm_add_state(m->alt.hmm, imm_super(m->special.alt.C));
    imm_hmm_add_state(m->alt.hmm, imm_super(m->special.alt.T));
    imm_hmm_set_start(m->alt.hmm, imm_super(m->special.alt.S), imm_log(1));
}

static void init_special_trans(struct dcp_pro_model *m)
{
    imm_float const one = imm_log(1.0);
    struct special_node *n = &m->special;

    struct imm_hmm *hmm = m->null.hmm;
    imm_hmm_set_trans(hmm, imm_super(n->null.R), imm_super(n->null.R), one);

    hmm = m->alt.hmm;
    imm_hmm_set_trans(hmm, imm_super(n->alt.S), imm_super(n->alt.B), one);
    imm_hmm_set_trans(hmm, imm_super(n->alt.S), imm_super(n->alt.N), one);
    imm_hmm_set_trans(hmm, imm_super(n->alt.N), imm_super(n->alt.N), one);
    imm_hmm_set_trans(hmm, imm_super(n->alt.N), imm_super(n->alt.B), one);

    imm_hmm_set_trans(hmm, imm_super(n->alt.E), imm_super(n->alt.T), one);
    imm_hmm_set_trans(hmm, imm_super(n->alt.E), imm_super(n->alt.C), one);
    imm_hmm_set_trans(hmm, imm_super(n->alt.C), imm_super(n->alt.C), one);
    imm_hmm_set_trans(hmm, imm_super(n->alt.C), imm_super(n->alt.T), one);

    imm_hmm_set_trans(hmm, imm_super(n->alt.E), imm_super(n->alt.B), one);
    imm_hmm_set_trans(hmm, imm_super(n->alt.E), imm_super(n->alt.J), one);
    imm_hmm_set_trans(hmm, imm_super(n->alt.J), imm_super(n->alt.J), one);
    imm_hmm_set_trans(hmm, imm_super(n->alt.J), imm_super(n->alt.B), one);
}
