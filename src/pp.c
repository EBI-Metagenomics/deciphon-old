#include "dcp/pp.h"
#include "imm/imm.h"
#include "support.h"
#include <assert.h>

#define super(x) imm_super((x))

struct special_transitions
{
    imm_float NN;
    imm_float NB;
    imm_float EC;
    imm_float CC;
    imm_float CT;
    imm_float EJ;
    imm_float JJ;
    imm_float JB;
    imm_float RR;
    imm_float BM;
    imm_float ME;
};

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
    struct imm_mute_state *S;
    struct imm_frame_state *N;
    struct imm_mute_state *B;
    struct imm_mute_state *E;
    struct imm_frame_state *J;
    struct imm_frame_state *C;
    struct imm_mute_state *T;
};

struct special_node_idx
{
    unsigned S;
    unsigned N;
    unsigned B;
    unsigned E;
    unsigned J;
    unsigned C;
    unsigned T;
};

struct dcp_pp
{
    struct imm_amino const *amino;
    struct imm_nuclt const *nuclt;
    unsigned core_size;
    imm_float epsilon;
    enum dcp_pp_entry_distr edist;
    struct special_transitions special_trans;

    struct
    {
        struct
        {
            imm_float lprobs[IMM_AMINO_SIZE];
            struct imm_nuclt_lprob nucltp;
            struct imm_codon_marg codonm;
            struct imm_frame_state *R;
            struct imm_hmm *hmm;
        } null;

        struct
        {
            struct special_node special;
            struct node *nodes;
            struct dcp_pp_transitions *trans;
            unsigned idx;
            struct imm_hmm *hmm;

            struct
            {
                struct imm_nuclt_lprob nucltp;
                struct imm_codon_marg codonm;
            } insert;
        } alt;
    } unpress;

    struct
    {
        struct
        {
            struct imm_dp *dp;
            unsigned R;
        } null;

        struct
        {
            struct special_node_idx special_idx;
            struct imm_dp *dp;
        } alt;
    } press;
};

void dcp_pp_state_name(unsigned id, char name[8])
{
    unsigned msb = id & (3U << (DCP_PP_BITS_ID - 2));
    if (msb == DCP_PP_SPECIAL_ID)
    {
        if (id == DCP_PP_R_ID)
            name[0] = 'R';
        else if (id == DCP_PP_S_ID)
            name[0] = 'S';
        else if (id == DCP_PP_N_ID)
            name[0] = 'N';
        else if (id == DCP_PP_B_ID)
            name[0] = 'B';
        else if (id == DCP_PP_E_ID)
            name[0] = 'E';
        else if (id == DCP_PP_J_ID)
            name[0] = 'J';
        else if (id == DCP_PP_C_ID)
            name[0] = 'C';
        else if (id == DCP_PP_T_ID)
            name[0] = 'T';
        name[1] = '\0';
    }
    else
    {
        if (msb == DCP_PP_MATCH_ID)
            name[0] = 'M';
        else if (msb == DCP_PP_INSERT_ID)
            name[0] = 'I';
        else if (msb == DCP_PP_DELETE_ID)
            name[0] = 'D';
        unsigned idx = id & (0xFFFF >> 2);
        snprintf(name + 1, 7, "%d", idx);
    }
}

static int setup_distributions(struct imm_amino const *amino,
                               imm_float const lprobs[IMM_AMINO_SIZE],
                               struct imm_nuclt_lprob *nucltp,
                               struct imm_codon_marg *codonm);

struct dcp_pp *dcp_pp_create(imm_float const null_lprobs[IMM_AMINO_SIZE],
                             imm_float const null_lodds[IMM_AMINO_SIZE],
                             unsigned core_size, struct dcp_pp_config cfg)
{
    struct dcp_pp *pp = xmalloc(sizeof(*pp));
    pp->epsilon = cfg.epsilon;
    pp->amino = &imm_amino_iupac;
    pp->nuclt = super(&imm_dna_default);

    pp->unpress.alt.hmm = NULL;
    pp->unpress.null.hmm = NULL;

    memcpy(pp->unpress.null.lprobs, null_lprobs,
           sizeof(*null_lprobs) * IMM_AMINO_SIZE);

    if (!(pp->unpress.null.hmm = imm_hmm_new(super(pp->nuclt))))
        goto cleanup;

    if (setup_distributions(pp->amino, null_lprobs, &pp->unpress.null.nucltp,
                            &pp->unpress.null.codonm))
        goto cleanup;

    if (!(pp->unpress.alt.hmm = imm_hmm_new(super(pp->nuclt))))
        goto cleanup;

    if (setup_distributions(pp->amino, null_lodds,
                            &pp->unpress.alt.insert.nucltp,
                            &pp->unpress.alt.insert.codonm))
        goto cleanup;

    imm_float e = pp->epsilon;
    struct imm_nuclt_lprob const *null_nucltp = &pp->unpress.null.nucltp;
    struct imm_codon_marg const *null_codonm = &pp->unpress.null.codonm;

    pp->unpress.null.R =
        imm_frame_state_new(DCP_PP_R_ID, null_nucltp, null_codonm, e);

    imm_hmm_add_state(pp->unpress.null.hmm, super(pp->unpress.null.R));
    imm_hmm_set_start(pp->unpress.null.hmm, super(pp->unpress.null.R),
                      imm_log(1));

    pp->unpress.alt.special.S =
        imm_mute_state_new(DCP_PP_S_ID, super(pp->nuclt));
    pp->unpress.alt.special.N =
        imm_frame_state_new(DCP_PP_N_ID, null_nucltp, null_codonm, e);
    pp->unpress.alt.special.B =
        imm_mute_state_new(DCP_PP_B_ID, super(pp->nuclt));
    pp->unpress.alt.special.E =
        imm_mute_state_new(DCP_PP_E_ID, super(pp->nuclt));
    pp->unpress.alt.special.J =
        imm_frame_state_new(DCP_PP_J_ID, null_nucltp, null_codonm, e);
    pp->unpress.alt.special.C =
        imm_frame_state_new(DCP_PP_C_ID, null_nucltp, null_codonm, e);
    pp->unpress.alt.special.T =
        imm_mute_state_new(DCP_PP_T_ID, super(pp->nuclt));

    imm_hmm_add_state(pp->unpress.alt.hmm, super(pp->unpress.alt.special.S));
    imm_hmm_add_state(pp->unpress.alt.hmm, super(pp->unpress.alt.special.N));
    imm_hmm_add_state(pp->unpress.alt.hmm, super(pp->unpress.alt.special.B));
    imm_hmm_add_state(pp->unpress.alt.hmm, super(pp->unpress.alt.special.E));
    imm_hmm_add_state(pp->unpress.alt.hmm, super(pp->unpress.alt.special.J));
    imm_hmm_add_state(pp->unpress.alt.hmm, super(pp->unpress.alt.special.C));
    imm_hmm_add_state(pp->unpress.alt.hmm, super(pp->unpress.alt.special.T));

    imm_hmm_set_start(pp->unpress.alt.hmm, super(pp->unpress.alt.special.S),
                      imm_log(1));

    pp->core_size = core_size;
    pp->unpress.alt.nodes = xcalloc(core_size, sizeof(*pp->unpress.alt.nodes));
    pp->unpress.alt.trans =
        xcalloc(core_size + 1, sizeof(*pp->unpress.alt.trans));
    pp->unpress.alt.idx = 0;
    pp->edist = cfg.edist;

    pp->special_trans.NN = imm_log(1);
    pp->special_trans.NB = imm_log(1);
    pp->special_trans.EC = imm_log(1);
    pp->special_trans.CC = imm_log(1);
    pp->special_trans.CT = imm_log(1);
    pp->special_trans.EJ = imm_log(1);
    pp->special_trans.JJ = imm_log(1);
    pp->special_trans.JB = imm_log(1);
    pp->special_trans.RR = imm_log(1);
    pp->special_trans.BM = imm_log(1);
    pp->special_trans.ME = imm_log(1);

    return pp;

cleanup:

    imm_del(pp->unpress.null.R);
    imm_del(pp->unpress.null.hmm);

    imm_del(pp->unpress.alt.special.S);
    imm_del(pp->unpress.alt.special.N);
    imm_del(pp->unpress.alt.special.B);
    imm_del(pp->unpress.alt.special.E);
    imm_del(pp->unpress.alt.special.J);
    imm_del(pp->unpress.alt.special.C);
    imm_del(pp->unpress.alt.special.T);
    imm_del(pp->unpress.alt.hmm);
    free(pp->unpress.alt.nodes);
    free(pp->unpress.alt.trans);
    free(pp);
    return NULL;
}

static struct imm_frame_state *new_match(struct dcp_pp *pp,
                                         struct imm_nuclt_lprob *nucltp,
                                         struct imm_codon_marg *codonm)
{
    imm_float e = pp->epsilon;
    unsigned id = DCP_PP_MATCH_ID | (pp->unpress.alt.idx + 1);
    struct imm_frame_state *frame = imm_frame_state_new(id, nucltp, codonm, e);
    return frame;
}

static struct imm_frame_state *new_insert(struct dcp_pp *pp)
{
    imm_float e = pp->epsilon;
    unsigned id = DCP_PP_INSERT_ID | (pp->unpress.alt.idx + 1);
    struct imm_nuclt_lprob *nucltp = &pp->unpress.alt.insert.nucltp;
    struct imm_codon_marg *codonm = &pp->unpress.alt.insert.codonm;
    struct imm_frame_state *frame = imm_frame_state_new(id, nucltp, codonm, e);
    return frame;
}

static struct imm_mute_state *new_delete(struct dcp_pp *pp)
{
    unsigned id = DCP_PP_DELETE_ID | (pp->unpress.alt.idx + 1);
    return imm_mute_state_new(id, super(pp->nuclt));
}

int dcp_pp_add_node(struct dcp_pp *pp, imm_float const lprobs[IMM_AMINO_SIZE])
{
    int rc = IMM_SUCCESS;

    imm_float lodds[IMM_AMINO_SIZE];
    for (unsigned i = 0; i < IMM_AMINO_SIZE; ++i)
        lodds[i] = lprobs[i] - pp->unpress.null.lprobs[i];

    struct node *node = pp->unpress.alt.nodes + pp->unpress.alt.idx;

    if ((rc = setup_distributions(pp->amino, lodds, &node->nucltp,
                                  &node->codonm)))
        return rc;

    struct imm_frame_state *M = new_match(pp, &node->nucltp, &node->codonm);

    if ((rc = imm_hmm_add_state(pp->unpress.alt.hmm, super(M))))
        return rc;

    struct imm_frame_state *ins = new_insert(pp);
    if ((rc = imm_hmm_add_state(pp->unpress.alt.hmm, super(ins))))
        return rc;

    struct imm_mute_state *del = new_delete(pp);
    if ((rc = imm_hmm_add_state(pp->unpress.alt.hmm, super(del))))
        return rc;

    node->M = M;
    node->I = ins;
    node->D = del;
    pp->unpress.alt.idx++;

    if (pp->unpress.alt.idx == pp->core_size)
        pp->unpress.alt.idx = 0;

    return rc;
}

static inline imm_float log1_p(imm_float logp)
{
    /* Compute log(1 - p) given log(p). */
    return log1p(-exp(logp));
}

static void calculate_occupancy(struct dcp_pp *pp, imm_float log_occ[static 1])
{
    struct dcp_pp_transitions *trans = pp->unpress.alt.trans;
    log_occ[0] = imm_lprob_add(trans->MI, trans->MM);
    for (unsigned i = 1; i < pp->core_size; ++i)
    {
        ++trans;
        imm_float val0 = log_occ[i - 1] + imm_lprob_add(trans->MM, trans->MI);
        imm_float val1 = log1_p(log_occ[i - 1]) + trans->DM;
        log_occ[i] = imm_lprob_add(val0, val1);
    }

    imm_float logZ = imm_lprob_zero();
    unsigned n = pp->core_size;
    for (unsigned i = 0; i < pp->core_size; ++i)
    {
        logZ = imm_lprob_add(logZ, log_occ[i] + imm_log(n - i));
    }

    for (unsigned i = 0; i < pp->core_size; ++i)
    {
        log_occ[i] -= logZ;
    }
}

static void setup_entry_transitions(struct dcp_pp *pp)
{
    if (pp->edist == UNIFORM)
    {
        imm_float M = (imm_float)pp->core_size;
        imm_float cost = imm_log(2.0 / (M * (M + 1))) * M;

        struct imm_state *B = super(pp->unpress.alt.special.B);
        for (unsigned i = 0; i < pp->core_size; ++i)
        {
            struct node node = pp->unpress.alt.nodes[i];
            imm_hmm_set_trans(pp->unpress.alt.hmm, B, super(node.M), cost);
        }
    }
    else
    {
        IMM_BUG(pp->edist != OCCUPANCY);
        imm_float *locc = xmalloc((pp->core_size) * sizeof(*locc));
        calculate_occupancy(pp, locc);
        struct imm_state *B = super(pp->unpress.alt.special.B);
        for (unsigned i = 0; i < pp->core_size; ++i)
        {
            struct node node = pp->unpress.alt.nodes[i];
            imm_hmm_set_trans(pp->unpress.alt.hmm, B, super(node.M), locc[i]);
        }
        free(locc);
    }
}

static void set_exit_transitions(struct dcp_pp *pp)
{
    struct imm_state *E = super(pp->unpress.alt.special.E);

    for (unsigned i = 0; i < pp->core_size; ++i)
    {
        struct node node = pp->unpress.alt.nodes[i];
        imm_hmm_set_trans(pp->unpress.alt.hmm, super(node.M), E, imm_log(1));
    }
    for (unsigned i = 1; i < pp->core_size; ++i)
    {
        struct node node = pp->unpress.alt.nodes[i];
        imm_hmm_set_trans(pp->unpress.alt.hmm, super(node.D), E, imm_log(1));
    }
}

static int setup_transitions(struct dcp_pp *pp)
{

    struct imm_hmm *hmm = pp->unpress.alt.hmm;
    struct dcp_pp_transitions *trans = pp->unpress.alt.trans;

    struct imm_state *B = super(pp->unpress.alt.special.B);
    struct imm_state *M1 = super(pp->unpress.alt.nodes[0].M);
    imm_hmm_set_trans(hmm, B, M1, trans[0].MM);

    for (unsigned i = 0; i + 1 < pp->core_size; ++i)
    {
        struct node prev = pp->unpress.alt.nodes[i];
        struct node next = pp->unpress.alt.nodes[i + 1];
        unsigned j = i + 1;
        imm_hmm_set_trans(hmm, super(prev.M), super(prev.I), trans[j].MI);
        imm_hmm_set_trans(hmm, super(prev.I), super(prev.I), trans[j].II);
        imm_hmm_set_trans(hmm, super(prev.M), super(next.M), trans[j].MM);
        imm_hmm_set_trans(hmm, super(prev.I), super(next.M), trans[j].IM);
        imm_hmm_set_trans(hmm, super(prev.M), super(next.D), trans[j].MD);
        imm_hmm_set_trans(hmm, super(prev.D), super(next.D), trans[j].DD);
        imm_hmm_set_trans(hmm, super(prev.D), super(next.M), trans[j].DM);
    }

    unsigned n = pp->core_size;
    struct imm_state *Mm = super(pp->unpress.alt.nodes[n - 1].M);
    imm_hmm_set_trans(hmm, Mm, super(pp->unpress.alt.special.E), trans[n].MM);

    setup_entry_transitions(pp);
    set_exit_transitions(pp);
    return IMM_SUCCESS;
}

static int setup_dp(struct dcp_pp *pp)
{
    int rc = IMM_SUCCESS;
    pp->press.null.dp =
        imm_hmm_new_dp(pp->unpress.null.hmm, super(pp->unpress.null.R));
    pp->press.alt.dp =
        imm_hmm_new_dp(pp->unpress.alt.hmm, super(pp->unpress.alt.special.T));

    pp->press.null.R = imm_state_idx(super(pp->unpress.null.R));

    pp->press.alt.special_idx.S =
        imm_state_idx(super(pp->unpress.alt.special.S));
    pp->press.alt.special_idx.N =
        imm_state_idx(super(pp->unpress.alt.special.N));
    pp->press.alt.special_idx.B =
        imm_state_idx(super(pp->unpress.alt.special.B));
    pp->press.alt.special_idx.E =
        imm_state_idx(super(pp->unpress.alt.special.E));
    pp->press.alt.special_idx.J =
        imm_state_idx(super(pp->unpress.alt.special.J));
    pp->press.alt.special_idx.C =
        imm_state_idx(super(pp->unpress.alt.special.C));
    pp->press.alt.special_idx.T =
        imm_state_idx(super(pp->unpress.alt.special.T));
    return rc;
}

int dcp_pp_add_trans(struct dcp_pp *pp, struct dcp_pp_transitions trans)
{
    int rc = IMM_SUCCESS;
    pp->unpress.alt.trans[pp->unpress.alt.idx++] = trans;
    if (pp->unpress.alt.idx == pp->core_size + 1)
    {
        if ((rc = setup_transitions(pp)))
            return rc;

        dcp_pp_set_target_length(pp, 1, true, false);

        if ((rc = setup_dp(pp)))
            return rc;
    }
    return rc;
}

struct imm_hmm *dcp_pp_null_hmm(struct dcp_pp *pp)
{
    return pp->unpress.null.hmm;
}

struct imm_hmm *dcp_pp_alt_hmm(struct dcp_pp *pp)
{
    return pp->unpress.alt.hmm;
}

struct imm_dp *dcp_pp_null_dp(struct dcp_pp *pp) { return pp->press.null.dp; }

struct imm_dp *dcp_pp_alt_dp(struct dcp_pp *pp) { return pp->press.alt.dp; }

void dcp_pp_destroy(struct dcp_pp *pp)
{
    if (pp)
    {
        imm_hmm_del(pp->unpress.alt.hmm);
        free(pp);
    }
}

static struct imm_codon_lprob setup_codonp(struct imm_amino const *amino,
                                           struct imm_amino_lprob const *aminop)
{
    /* FIXME: We don't need 255 positions*/
    unsigned count[] = FILL(255, 0);

    for (unsigned i = 0; i < imm_gc_size(); ++i)
        count[(unsigned)imm_gc_aa(1, i)] += 1;

    struct imm_abc const *abc = super(amino);
    /* TODO: We don't need 255 positions*/
    imm_float lprobs[] = FILL(255, IMM_LPROB_ZERO);
    for (unsigned i = 0; i < imm_abc_size(abc); ++i)
    {
        char aa = imm_abc_symbols(abc)[i];
        imm_float norm = imm_log((imm_float)count[(unsigned)aa]);
        lprobs[(unsigned)aa] = imm_amino_lprob_get(aminop, aa) - norm;
    }

    struct imm_codon_lprob codonp = imm_codon_lprob(super(imm_gc_dna()));
    for (unsigned i = 0; i < imm_gc_size(); ++i)
    {
        char aa = imm_gc_aa(1, i);
        imm_codon_lprob_set(&codonp, imm_gc_codon(1, i), lprobs[(unsigned)aa]);
#if 0
        struct imm_codon codon = imm_gc_codon(1, i);
        printf("[%c%c%c] %f\n",
               imm_abc_symbols(super(codon.nuclt))[codon.a],
               imm_abc_symbols(super(codon.nuclt))[codon.b],
               imm_abc_symbols(super(codon.nuclt))[codon.c],
               lprobs[(unsigned)aa]);
#endif
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

static int setup_distributions(struct imm_amino const *amino,
                               imm_float const lprobs[IMM_AMINO_SIZE],
                               struct imm_nuclt_lprob *nucltp,
                               struct imm_codon_marg *codonm)
{
    int rc = IMM_SUCCESS;
    struct imm_amino_lprob aminop = imm_amino_lprob(amino, lprobs);
    struct imm_codon_lprob codonp = setup_codonp(amino, &aminop);
    if ((rc = imm_codon_lprob_normalize(&codonp)))
        return rc;

    *nucltp = setup_nucltp(&codonp);
    *codonm = imm_codon_marg(&codonp);
    return rc;
}

void dcp_pp_set_target_length(struct dcp_pp *pp, unsigned seq_len,
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

    /* special_trans */
    struct special_transitions *t = &pp->special_trans;

    t->NN = t->CC = t->JJ = lp;
    t->NB = t->CT = t->JB = l1p;
    t->RR = lr;
    t->EJ = log_q;
    t->EC = imm_log(1 - q);

    if (hmmer3_compat)
    {
        t->NN = t->CC = t->JJ = imm_log(1);
    }

    imm_hmm_set_trans(pp->unpress.null.hmm, super(pp->unpress.null.R),
                      super(pp->unpress.null.R), t->RR);

    struct special_node *s = &pp->unpress.alt.special;

    struct imm_hmm *ahmm = pp->unpress.alt.hmm;
    imm_hmm_set_trans(ahmm, super(s->S), super(s->B), t->NB);
    imm_hmm_set_trans(ahmm, super(s->S), super(s->N), t->NN);
    imm_hmm_set_trans(ahmm, super(s->N), super(s->N), t->NN);
    imm_hmm_set_trans(ahmm, super(s->N), super(s->B), t->NB);

    imm_hmm_set_trans(ahmm, super(s->E), super(s->T), t->EC + t->CT);
    imm_hmm_set_trans(ahmm, super(s->E), super(s->C), t->EC + t->CC);
    imm_hmm_set_trans(ahmm, super(s->C), super(s->C), t->CC);
    imm_hmm_set_trans(ahmm, super(s->C), super(s->T), t->CT);

    imm_hmm_set_trans(ahmm, super(s->E), super(s->B), t->EJ + t->JB);
    imm_hmm_set_trans(ahmm, super(s->E), super(s->J), t->EJ + t->JJ);
    imm_hmm_set_trans(ahmm, super(s->J), super(s->J), t->JJ);
    imm_hmm_set_trans(ahmm, super(s->J), super(s->B), t->JB);
}

void dcp_pp_set_target_length2(struct dcp_pp *pp, unsigned seq_len,
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

    /* special_trans */
    struct special_transitions *t = &pp->special_trans;

    t->NN = t->CC = t->JJ = lp;
    t->NB = t->CT = t->JB = l1p;
    t->RR = lr;
    t->EJ = log_q;
    t->EC = imm_log(1 - q);

    if (hmmer3_compat)
    {
        t->NN = t->CC = t->JJ = imm_log(1);
    }

    struct imm_dp *dp = pp->press.null.dp;
    unsigned R = pp->press.null.R;
    imm_dp_change_trans(dp, imm_dp_trans_idx(dp, R, R), t->RR);

    dp = pp->press.alt.dp;
    struct special_node_idx *n = &pp->press.alt.special_idx;

    imm_dp_change_trans(dp, imm_dp_trans_idx(dp, n->S, n->B), t->NB);
    imm_dp_change_trans(dp, imm_dp_trans_idx(dp, n->S, n->N), t->NN);
    imm_dp_change_trans(dp, imm_dp_trans_idx(dp, n->N, n->N), t->NN);
    imm_dp_change_trans(dp, imm_dp_trans_idx(dp, n->N, n->B), t->NB);

    imm_dp_change_trans(dp, imm_dp_trans_idx(dp, n->E, n->T), t->EC + t->CT);
    imm_dp_change_trans(dp, imm_dp_trans_idx(dp, n->E, n->C), t->EC + t->CC);
    imm_dp_change_trans(dp, imm_dp_trans_idx(dp, n->C, n->C), t->CC);
    imm_dp_change_trans(dp, imm_dp_trans_idx(dp, n->C, n->T), t->CT);

    imm_dp_change_trans(dp, imm_dp_trans_idx(dp, n->E, n->B), t->EC + t->CT);
    imm_dp_change_trans(dp, imm_dp_trans_idx(dp, n->E, n->J), t->EC + t->CC);
    imm_dp_change_trans(dp, imm_dp_trans_idx(dp, n->J, n->J), t->CC);
    imm_dp_change_trans(dp, imm_dp_trans_idx(dp, n->J, n->B), t->CT);
}
