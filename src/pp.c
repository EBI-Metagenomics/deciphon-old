#include "dcp/pp.h"
#include "imm/imm.h"
#include "support.h"
#include <assert.h>

#define BITS_PER_STATE_ID 16

#define MATCH_ID (0U << (BITS_PER_STATE_ID - 2))
#define INSERT_ID (1U << (BITS_PER_STATE_ID - 2))
#define DELETE_ID (2U << (BITS_PER_STATE_ID - 2))
#define SPECIAL_ID (3U << (BITS_PER_STATE_ID - 2))
#define R_ID (SPECIAL_ID | 0U)
#define S_ID (SPECIAL_ID | 1U)
#define N_ID (SPECIAL_ID | 2U)
#define B_ID (SPECIAL_ID | 3U)
#define E_ID (SPECIAL_ID | 4U)
#define J_ID (SPECIAL_ID | 5U)
#define C_ID (SPECIAL_ID | 6U)
#define T_ID (SPECIAL_ID | 7U)

#define super(x) imm_super((x))

void dcp_pp_state_name(unsigned id, char name[8])
{
    unsigned msb = id & (3U << (BITS_PER_STATE_ID - 2));
    if (msb == SPECIAL_ID)
    {
        if (id == R_ID)
        {
            name[0] = 'R';
        }
        else if (id == S_ID)
        {
            name[0] = 'S';
        }
        else if (id == N_ID)
        {
            name[0] = 'N';
        }
        else if (id == B_ID)
        {
            name[0] = 'B';
        }
        else if (id == E_ID)
        {
            name[0] = 'E';
        }
        else if (id == J_ID)
        {
            name[0] = 'J';
        }
        else if (id == C_ID)
        {
            name[0] = 'C';
        }
        else if (id == T_ID)
        {
            name[0] = 'T';
        }
        name[1] = '\0';
    }
    else
    {
        if (msb == MATCH_ID)
        {
            name[0] = 'M';
        }
        else if (msb == INSERT_ID)
        {
            name[0] = 'I';
        }
        else if (msb == DELETE_ID)
        {
            name[0] = 'D';
        }
        unsigned idx = id & (0xFFFF >> 2);
        snprintf(name + 1, 7, "%d", idx);
    }
}

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

struct dcp_pp
{
    imm_float epsilon;
    struct imm_amino const *amino;
    struct imm_nuclt const *nuclt;

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
        unsigned core_size;
        struct node *nodes;
        struct dcp_trans *trans;
        unsigned idx;

        struct imm_hmm *hmm;
        enum dcp_entry_distr entry_distr;

        struct
        {
            struct imm_nuclt_lprob nucltp;
            struct imm_codon_marg codonm;
        } insert;
    } alt;

    struct dcp_special_trans special_trans;
};

static int setup_distributions(struct imm_amino const *amino,
                               imm_float const lprobs[IMM_AMINO_SIZE],
                               struct imm_nuclt_lprob *nucltp,
                               struct imm_codon_marg *codonm);

struct dcp_pp *dcp_pp_create(imm_float const null_lprobs[IMM_AMINO_SIZE],
                             imm_float const null_lodds[IMM_AMINO_SIZE],
                             imm_float epsilon, unsigned core_size,
                             enum dcp_entry_distr entry_distr)
{
    struct dcp_pp *pp = xmalloc(sizeof(*pp));
    pp->epsilon = epsilon;
    pp->amino = &imm_amino_iupac;
    pp->nuclt = imm_super(&imm_dna_default);

    pp->alt.hmm = NULL;
    pp->null.hmm = NULL;

    memcpy(pp->null.lprobs, null_lprobs, sizeof(*null_lprobs) * IMM_AMINO_SIZE);

    if (!(pp->null.hmm = imm_hmm_new(imm_super(pp->nuclt))))
        goto cleanup;

    if (setup_distributions(pp->amino, null_lprobs, &pp->null.nucltp,
                            &pp->null.codonm))
        goto cleanup;

    if (!(pp->alt.hmm = imm_hmm_new(imm_super(pp->nuclt))))
        goto cleanup;

    if (setup_distributions(pp->amino, null_lodds, &pp->alt.insert.nucltp,
                            &pp->alt.insert.codonm))
        goto cleanup;

    imm_float e = epsilon;
    struct imm_nuclt_lprob const *null_nucltp = &pp->null.nucltp;
    struct imm_codon_marg const *null_codonm = &pp->null.codonm;

    pp->null.R = imm_frame_state_new(R_ID, null_nucltp, null_codonm, e);

    imm_hmm_add_state(pp->null.hmm, imm_super(pp->null.R));
    imm_hmm_set_start(pp->null.hmm, imm_super(pp->null.R), imm_log(1));

    pp->alt.special.S = imm_mute_state_new(S_ID, imm_super(pp->nuclt));
    pp->alt.special.N = imm_frame_state_new(N_ID, null_nucltp, null_codonm, e);
    pp->alt.special.B = imm_mute_state_new(B_ID, imm_super(pp->nuclt));
    pp->alt.special.E = imm_mute_state_new(E_ID, imm_super(pp->nuclt));
    pp->alt.special.J = imm_frame_state_new(J_ID, null_nucltp, null_codonm, e);
    pp->alt.special.C = imm_frame_state_new(C_ID, null_nucltp, null_codonm, e);
    pp->alt.special.T = imm_mute_state_new(T_ID, imm_super(pp->nuclt));

    imm_hmm_add_state(pp->alt.hmm, imm_super(pp->alt.special.S));
    imm_hmm_add_state(pp->alt.hmm, imm_super(pp->alt.special.N));
    imm_hmm_add_state(pp->alt.hmm, imm_super(pp->alt.special.B));
    imm_hmm_add_state(pp->alt.hmm, imm_super(pp->alt.special.E));
    imm_hmm_add_state(pp->alt.hmm, imm_super(pp->alt.special.J));
    imm_hmm_add_state(pp->alt.hmm, imm_super(pp->alt.special.C));
    imm_hmm_add_state(pp->alt.hmm, imm_super(pp->alt.special.T));

    imm_hmm_set_start(pp->alt.hmm, imm_super(pp->alt.special.S), imm_log(1));

    pp->alt.core_size = core_size;
    pp->alt.nodes = xcalloc(core_size, sizeof(*pp->alt.nodes));
    pp->alt.trans = xcalloc(core_size + 1, sizeof(*pp->alt.trans));
    pp->alt.idx = 0;
    pp->alt.entry_distr = entry_distr;

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

    imm_del(pp->null.R);
    imm_del(pp->null.hmm);

    imm_del(pp->alt.special.S);
    imm_del(pp->alt.special.N);
    imm_del(pp->alt.special.B);
    imm_del(pp->alt.special.E);
    imm_del(pp->alt.special.J);
    imm_del(pp->alt.special.C);
    imm_del(pp->alt.special.T);
    imm_del(pp->alt.hmm);
    free(pp->alt.nodes);
    free(pp->alt.trans);
    free(pp);
    return NULL;
}

static void show_frame(unsigned id, struct imm_frame_state *frame)
{
    char letters[] = "ACGT";
    char str[8];
    for (unsigned i0 = 0; i0 < 4; ++i0)
    {
        str[0] = letters[i0];
        str[1] = '\0';
        struct imm_seq seq =
            imm_seq(imm_str(str), imm_super(imm_super(imm_gc_dna())));

        char name[8];
        dcp_pp_state_name(id, name);
        printf("%s %s %f\n", name, str,
               imm_state_lprob(imm_super(frame), &seq));
    }
    for (unsigned i0 = 0; i0 < 4; ++i0)
    {
        for (unsigned i1 = 0; i1 < 4; ++i1)
        {
            str[0] = letters[i0];
            str[1] = letters[i1];
            str[2] = '\0';
            struct imm_seq seq =
                imm_seq(imm_str(str), imm_super(imm_super(imm_gc_dna())));

            char name[8];
            dcp_pp_state_name(id, name);
            printf("%s %s %f\n", name, str,
                   imm_state_lprob(imm_super(frame), &seq));
        }
    }
    for (unsigned i0 = 0; i0 < 4; ++i0)
    {
        for (unsigned i1 = 0; i1 < 4; ++i1)
        {
            for (unsigned i2 = 0; i2 < 4; ++i2)
            {
                str[0] = letters[i0];
                str[1] = letters[i1];
                str[2] = letters[i2];
                str[3] = '\0';
                struct imm_seq seq =
                    imm_seq(imm_str(str), imm_super(imm_super(imm_gc_dna())));

                char name[8];
                dcp_pp_state_name(id, name);
                printf("%s %s %f\n", name, str,
                       imm_state_lprob(imm_super(frame), &seq));
            }
        }
    }
    for (unsigned i0 = 0; i0 < 4; ++i0)
    {
        for (unsigned i1 = 0; i1 < 4; ++i1)
        {
            for (unsigned i2 = 0; i2 < 4; ++i2)
            {
                for (unsigned i3 = 0; i3 < 4; ++i3)
                {
                    str[0] = letters[i0];
                    str[1] = letters[i1];
                    str[2] = letters[i2];
                    str[3] = letters[i3];
                    str[4] = '\0';
                    struct imm_seq seq = imm_seq(
                        imm_str(str), imm_super(imm_super(imm_gc_dna())));

                    char name[8];
                    dcp_pp_state_name(id, name);
                    printf("%s %s %f\n", name, str,
                           imm_state_lprob(imm_super(frame), &seq));
                }
            }
        }
    }
    for (unsigned i0 = 0; i0 < 4; ++i0)
    {
        for (unsigned i1 = 0; i1 < 4; ++i1)
        {
            for (unsigned i2 = 0; i2 < 4; ++i2)
            {
                for (unsigned i3 = 0; i3 < 4; ++i3)
                {
                    for (unsigned i4 = 0; i4 < 4; ++i4)
                    {
                        str[0] = letters[i0];
                        str[1] = letters[i1];
                        str[2] = letters[i2];
                        str[3] = letters[i3];
                        str[4] = letters[i4];
                        str[5] = '\0';
                        struct imm_seq seq = imm_seq(
                            imm_str(str), imm_super(imm_super(imm_gc_dna())));

                        char name[8];
                        dcp_pp_state_name(id, name);
                        printf("%s %s %f\n", name, str,
                               imm_state_lprob(imm_super(frame), &seq));
                    }
                }
            }
        }
    }
}

static struct imm_frame_state *new_match(struct dcp_pp *pp,
                                         struct imm_nuclt_lprob *nucltp,
                                         struct imm_codon_marg *codonm)
{
    imm_float e = pp->epsilon;
    unsigned id = MATCH_ID | (pp->alt.idx + 1);
    struct imm_frame_state *frame = imm_frame_state_new(id, nucltp, codonm, e);
    show_frame(id, frame);
    return frame;
}

static struct imm_frame_state *new_insert(struct dcp_pp *pp)
{
    imm_float e = pp->epsilon;
    unsigned id = INSERT_ID | (pp->alt.idx + 1);
    struct imm_nuclt_lprob *nucltp = &pp->alt.insert.nucltp;
    struct imm_codon_marg *codonm = &pp->alt.insert.codonm;
    struct imm_frame_state *frame = imm_frame_state_new(id, nucltp, codonm, e);
    show_frame(id, frame);
    return frame;
}

static struct imm_mute_state *new_delete(struct dcp_pp *pp)
{
    unsigned id = DELETE_ID | (pp->alt.idx + 1);
    return imm_mute_state_new(id, imm_super(pp->nuclt));
}

int dcp_pp_add_node(struct dcp_pp *pp, imm_float const lprobs[IMM_AMINO_SIZE])
{
    int rc = IMM_SUCCESS;

    imm_float lodds[IMM_AMINO_SIZE];
    for (unsigned i = 0; i < IMM_AMINO_SIZE; ++i)
        lodds[i] = lprobs[i] - pp->null.lprobs[i];

    struct node *node = pp->alt.nodes + pp->alt.idx;

    if ((rc = setup_distributions(pp->amino, lodds, &node->nucltp,
                                  &node->codonm)))
        return rc;

    struct imm_frame_state *M = new_match(pp, &node->nucltp, &node->codonm);

    if ((rc = imm_hmm_add_state(pp->alt.hmm, imm_super(M))))
        return rc;

    struct imm_frame_state *ins = new_insert(pp);
    if ((rc = imm_hmm_add_state(pp->alt.hmm, imm_super(ins))))
        return rc;

    struct imm_mute_state *del = new_delete(pp);
    if ((rc = imm_hmm_add_state(pp->alt.hmm, imm_super(del))))
        return rc;

    node->M = M;
    node->I = ins;
    node->D = del;
    pp->alt.idx++;

    if (pp->alt.idx == pp->alt.core_size)
        pp->alt.idx = 0;

    return rc;
}

static inline imm_float log1_p(imm_float logp)
{
    /* Compute log(1 - p) given log(p). */
    return log1p(-exp(logp));
}

static void calculate_occupancy(struct dcp_pp *pp, imm_float log_occ[static 1])
{
    struct dcp_trans *trans = pp->alt.trans;
    log_occ[0] = imm_lprob_add(trans->MI, trans->MM);
    for (unsigned i = 1; i < pp->alt.core_size; ++i)
    {
        ++trans;
        /* trans = pp->alt.trans + i; */
        imm_float val0 = log_occ[i - 1] + imm_lprob_add(trans->MM, trans->MI);
        imm_float val1 = log1_p(log_occ[i - 1]) + trans->DM;
        log_occ[i] = imm_lprob_add(val0, val1);
    }

    imm_float logZ = imm_lprob_zero();
    unsigned n = pp->alt.core_size;
    for (unsigned i = 0; i < pp->alt.core_size; ++i)
    {
        logZ = imm_lprob_add(logZ, log_occ[i] + imm_log(n - i));
    }

    for (unsigned i = 0; i < pp->alt.core_size; ++i)
    {
        log_occ[i] -= logZ;
    }
}

static void setup_entry_transitions(struct dcp_pp *pp)
{
    if (pp->alt.entry_distr == UNIFORM)
    {
        imm_float M = (imm_float)pp->alt.core_size;
        imm_float cost = imm_log(2.0 / (M * (M + 1))) * M;

        struct imm_state *B = imm_super(pp->alt.special.B);
        for (unsigned i = 0; i < pp->alt.core_size; ++i)
        {
            struct node node = pp->alt.nodes[i];
            imm_hmm_set_trans(pp->alt.hmm, B, imm_super(node.M), cost);
        }
    }
    else
    {
        IMM_BUG(pp->alt.entry_distr != OCCUPANCY);
        imm_float *locc = xmalloc((pp->alt.core_size) * sizeof(*locc));
        calculate_occupancy(pp, locc);
        struct imm_state *B = imm_super(pp->alt.special.B);
        for (unsigned i = 0; i < pp->alt.core_size; ++i)
        {
            struct node node = pp->alt.nodes[i];
            imm_hmm_set_trans(pp->alt.hmm, B, imm_super(node.M), locc[i]);
        }
        free(locc);
    }
}

static void set_exit_transitions(struct dcp_pp *pp)
{
    struct imm_state *E = imm_super(pp->alt.special.E);

    for (unsigned i = 0; i < pp->alt.core_size; ++i)
    {
        struct node node = pp->alt.nodes[i];
        imm_hmm_set_trans(pp->alt.hmm, imm_super(node.M), E, imm_log(1));
    }
    for (unsigned i = 1; i < pp->alt.core_size; ++i)
    {
        struct node node = pp->alt.nodes[i];
        imm_hmm_set_trans(pp->alt.hmm, imm_super(node.D), E, imm_log(1));
    }
}

static int setup_transitions(struct dcp_pp *pp)
{

    struct imm_hmm *hmm = pp->alt.hmm;
    struct dcp_trans *trans = pp->alt.trans;

    struct imm_state *B = super(pp->alt.special.B);
    struct imm_state *M1 = super(pp->alt.nodes[0].M);
    imm_hmm_set_trans(hmm, B, M1, trans[0].MM);

    for (unsigned i = 0; i + 1 < pp->alt.core_size; ++i)
    {
        struct node prev = pp->alt.nodes[i];
        struct node next = pp->alt.nodes[i + 1];
        unsigned j = i + 1;
        imm_hmm_set_trans(hmm, super(prev.M), super(prev.I), trans[j].MI);
        imm_hmm_set_trans(hmm, super(prev.I), super(prev.I), trans[j].II);
        imm_hmm_set_trans(hmm, super(prev.M), super(next.M), trans[j].MM);
        imm_hmm_set_trans(hmm, super(prev.I), super(next.M), trans[j].IM);
        imm_hmm_set_trans(hmm, super(prev.M), super(next.D), trans[j].MD);
        imm_hmm_set_trans(hmm, super(prev.D), super(next.D), trans[j].DD);
        imm_hmm_set_trans(hmm, super(prev.D), super(next.M), trans[j].DM);
    }

    unsigned n = pp->alt.core_size;
    struct imm_state *Mm = super(pp->alt.nodes[n - 1].M);
    imm_hmm_set_trans(hmm, Mm, super(pp->alt.special.E), trans[n].MM);

    /* alt_model.set_entry_transitions(entry_distr, core_trans) */

    setup_entry_transitions(pp);
    set_exit_transitions(pp);
    return IMM_SUCCESS;
}

int dcp_pp_add_trans(struct dcp_pp *pp, struct dcp_trans trans)
{
    pp->alt.trans[pp->alt.idx++] = trans;
    if (pp->alt.idx == pp->alt.core_size + 1)
    {
        return setup_transitions(pp);
    }
    return IMM_SUCCESS;
}

struct imm_hmm *dcp_pp_null_hmm(struct dcp_pp *pp) { return pp->null.hmm; }

struct imm_hmm *dcp_pp_alt_hmm(struct dcp_pp *pp) { return pp->alt.hmm; }

struct imm_dp *dcp_pp_null_new_dp(struct dcp_pp *pp)
{
    return imm_hmm_new_dp(pp->null.hmm, super(pp->null.R));
}

struct imm_dp *dcp_pp_alt_new_dp(struct dcp_pp *pp)
{
    return imm_hmm_new_dp(pp->alt.hmm, super(pp->alt.special.T));
}

void dcp_pp_destroy(struct dcp_pp *pp)
{
    if (pp)
    {
        imm_hmm_del(pp->alt.hmm);
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

    struct imm_abc const *abc = imm_super(amino);
    /* TODO: We don't need 255 positions*/
    imm_float lprobs[] = FILL(255, IMM_LPROB_ZERO);
    for (unsigned i = 0; i < imm_abc_size(abc); ++i)
    {
        char aa = imm_abc_symbols(abc)[i];
        imm_float norm = imm_log((imm_float)count[(unsigned)aa]);
        lprobs[(unsigned)aa] = imm_amino_lprob_get(aminop, aa) - norm;
    }

    struct imm_codon_lprob codonp = imm_codon_lprob(imm_super(imm_gc_dna()));
    for (unsigned i = 0; i < imm_gc_size(); ++i)
    {
        char aa = imm_gc_aa(1, i);
        imm_codon_lprob_set(&codonp, imm_gc_codon(1, i), lprobs[(unsigned)aa]);
#if 0
        struct imm_codon codon = imm_gc_codon(1, i);
        printf("[%c%c%c] %f\n",
               imm_abc_symbols(imm_super(codon.nuclt))[codon.a],
               imm_abc_symbols(imm_super(codon.nuclt))[codon.b],
               imm_abc_symbols(imm_super(codon.nuclt))[codon.c],
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

void dcp_pp_set_target_length(struct dcp_pp *pp, unsigned target_length,
                              bool multihits, bool hmmer3_compat)
{
    imm_float L = (imm_float)target_length;
    /* if (L == 0): */
    /*     raise ValueError("Target length cannot be zero.") */

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
    struct dcp_special_trans *t = &pp->special_trans;

    t->NN = t->CC = t->JJ = lp;
    t->NB = t->CT = t->JB = l1p;
    t->RR = lr;
    t->EJ = log_q;
    t->EC = imm_log(1 - q);

    if (hmmer3_compat)
    {
        t->NN = t->CC = t->JJ = 0.0;
    }

    imm_hmm_set_trans(pp->null.hmm, super(pp->null.R), super(pp->null.R),
                      t->RR);

    struct special_node *s = &pp->alt.special;

    struct imm_hmm *ahmm = pp->alt.hmm;
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

    show_frame(imm_state_id(imm_super(s->N)), s->N);
    show_frame(imm_state_id(imm_super(s->J)), s->J);
    show_frame(imm_state_id(imm_super(s->C)), s->C);

#if 0
    struct node *nodes = pp->alt.nodes;
    /* TODO: if fails for HMM having one core state */
    imm_hmm_set_trans(ahmm, super(s->B), super(nodes[1].D), IMM_LPROB_ZERO);
    imm_hmm_set_trans(ahmm, super(s->B), super(nodes[0].I), IMM_LPROB_ZERO);
#endif
}
