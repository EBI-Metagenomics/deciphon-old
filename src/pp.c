#include "dcp/pp.h"
#include "imm/imm.h"
#include "support.h"
#include <assert.h>

#define MATCH_ID (1U << (BITS_PER_INT - 1))
#define INSERT_ID (2U << (BITS_PER_INT - 1))
#define DELETE_ID (3U << (BITS_PER_INT - 1))
#define R_ID (UINT_MAX)

struct node
{
    struct imm_frame_state *match;
    struct imm_frame_state *insert;
    struct imm_mute_state *delete;
};

struct dcp_pp
{
    struct imm_amino const *amino;
    struct imm_nuclt const *nuclt;
    struct imm_hmm *hmm;

    struct
    {
        struct imm_nuclt_lprob nucltp;
        struct imm_codon_marg codonm;
        struct imm_frame_state *R;
    } null;

    struct
    {
        struct
        {
            struct imm_mute_state *S;
            struct imm_frame_state *N;
            struct imm_mute_state *B;
            struct imm_mute_state *E;
            struct imm_frame_state *J;
            struct imm_frame_state *C;
            struct imm_mute_state *T;
        } special;

        unsigned nnodes;
        size_t capacity;
        struct node *nodes;
        unsigned curr_idx;
    } alt;

    imm_float epsilon;
};

static int setup_distributions(struct imm_amino const *amino,
                               imm_float const lprobs[IMM_AMINO_SIZE],
                               struct imm_nuclt_lprob *nucltp,
                               struct imm_codon_marg *codonm);

struct dcp_pp *dcp_pp_create(imm_float const null_lprobs[IMM_AMINO_SIZE],
                             imm_float const null_lodds[IMM_AMINO_SIZE])
{
    struct dcp_pp *pp = xmalloc(sizeof(*pp));
    pp->amino = &imm_amino_iupac;
    pp->nuclt = imm_super(&imm_dna_default);
    if (!(pp->hmm = imm_hmm_new(imm_super(pp->amino))))
        goto cleanup;
    pp->alt.curr_idx = 0;

    if (setup_distributions(pp->amino, null_lodds, &pp->null.nucltp,
                            &pp->null.codonm))
        goto cleanup;

    pp->alt.nnodes = 0;
    pp->alt.capacity = 1 << 8;
    pp->alt.nodes = xcalloc(pp->alt.capacity, sizeof(*pp->alt.nodes));

    return pp;

cleanup:
    if (pp->hmm)
        imm_hmm_del(pp->hmm);
    return NULL;
}

static struct imm_frame_state *new_match(struct dcp_pp *pp,
                                         imm_float const lodds[IMM_AMINO_SIZE])
{
    struct imm_nuclt_lprob nucltp;
    struct imm_codon_marg codonm;
    if (setup_distributions(pp->amino, lodds, &nucltp, &codonm))
        return NULL;

    imm_float e = pp->epsilon;
    unsigned id = MATCH_ID | pp->alt.curr_idx;
    return imm_frame_state_new(id, &nucltp, &codonm, e);
}

static struct imm_frame_state *new_insert(struct dcp_pp *pp)
{
    imm_float e = pp->epsilon;
    unsigned id = INSERT_ID | pp->alt.curr_idx;
    return imm_frame_state_new(id, &pp->null.nucltp, &pp->null.codonm, e);
}

static struct imm_mute_state *new_delete(struct dcp_pp *pp)
{
    unsigned id = DELETE_ID | pp->alt.curr_idx;
    return imm_mute_state_new(id, imm_super(pp->nuclt));
}

int dcp_pp_add_node(struct dcp_pp *pp, imm_float const lodds[static 1])
{
    int rc = IMM_SUCCESS;

    struct imm_frame_state *match = new_match(pp, lodds);
    if ((rc = imm_hmm_add_state(pp->hmm, imm_super(match))))
        return rc;

    struct imm_frame_state *ins = new_insert(pp);
    if ((rc = imm_hmm_add_state(pp->hmm, imm_super(ins))))
        return rc;

    struct imm_mute_state *del = new_delete(pp);
    if ((rc = imm_hmm_add_state(pp->hmm, imm_super(del))))
        return rc;

    pp->alt.nodes[pp->alt.curr_idx].match = match;
    pp->alt.nodes[pp->alt.curr_idx].insert = ins;
    pp->alt.nodes[pp->alt.curr_idx].delete = del;
    pp->alt.curr_idx++;
    pp->alt.nnodes++;

    if (pp->alt.nnodes > pp->alt.capacity)
    {
        pp->alt.capacity <<= 1;
        pp->alt.nodes =
            xrealloc(pp->alt.nodes, pp->alt.capacity * sizeof(*pp->alt.nodes));
    }
    return rc;
}

void dcp_pp_destroy(struct dcp_pp *pp)
{
    if (pp->hmm)
        imm_hmm_del(pp->hmm);
    free(pp);
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
    imm_float lprobs[] = FILL(255, 0);
    for (unsigned i = 0; i < imm_abc_size(abc); ++i)
    {
        char aa = imm_abc_symbols(abc)[i];
        imm_float norm = imm_log((imm_float)count[(unsigned)aa]);
        lprobs[(unsigned)aa] = imm_amino_lprob_get(aminop, aa) - norm;
    }

    struct imm_codon_lprob codonp;
    for (unsigned i = 0; i < imm_gc_size(); ++i)
    {
        char aa = imm_gc_aa(1, i);
        imm_codon_lprob_set(&codonp, imm_gc_codon(1, i), lprobs[(unsigned)aa]);
    }
    return codonp;
}

static struct imm_nuclt_lprob setup_nucltp(struct imm_codon_lprob const *codonp)
{
    imm_float lprobs[] = FILL(IMM_NUCLT_SIZE, imm_lprob_zero());

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
