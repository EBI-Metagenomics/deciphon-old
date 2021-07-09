#include "dcp/pp.h"
#include "imm/imm.h"
#include "support.h"
#include <assert.h>

#define MATCH_ID (1U << (BITS_PER_INT - 1))
#define INSERT_ID (2U << (BITS_PER_INT - 1))
#define DELETE_ID (3U << (BITS_PER_INT - 1))

struct dcp_pp
{
    struct imm_amino const *amino;
    struct imm_nuclt const *nuclt;
    struct imm_hmm *hmm;
    unsigned curr_node_idx;
    struct imm_nuclt_lprob nucltp;

    struct
    {
        struct imm_amino_lprob aminop;
        struct imm_codon_lprob codonp;
        struct imm_codon_marg const *codonm;
    } null;

    struct
    {
        struct imm_amino_lprob aminop;
        struct imm_codon_lprob codonp;
        struct imm_codon_marg const *codonm;
    } alt;
    imm_float epsilon;
};

struct dcp_pp *dcp_pp_create(imm_float const null_lprobs[IMM_AMINO_SIZE])
{
    struct dcp_pp *pp = xmalloc(sizeof(*pp));
    pp->amino = &imm_amino_default;
    pp->nuclt = imm_super(&imm_dna_default);
    pp->hmm = imm_hmm_new(imm_super(pp->amino));
    pp->curr_node_idx = 0;

    static_assert(IMM_NUCLT_SIZE == 4, "DNA or RNA letters.");
    imm_float const lprobs[IMM_NUCLT_SIZE] = {-imm_log(4), -imm_log(4),
                                              -imm_log(4), -imm_log(4)};
    pp->nucltp = imm_nuclt_lprob(pp->nuclt, lprobs);

    pp->null.aminop = imm_amino_lprob(pp->amino, null_lprobs);
    pp->null.codonp = imm_codon_lprob(pp->nuclt);
    pp->null.codonm = NULL;

    pp->alt.aminop = imm_amino_lprob(pp->amino, null_lprobs);
    pp->alt.codonp = imm_codon_lprob(pp->nuclt);
    pp->alt.codonm = NULL;

    return pp;
}

int set_codonp(struct imm_amino const *amino,
               struct imm_amino_lprob const *aminop,
               struct imm_codon_lprob *codonp)
{
    /* TODO: We don't need 255 positions*/
    unsigned count[255] = {0};

    for (unsigned i = 0; i < imm_gc_size(); ++i)
        count[(unsigned)imm_gc_aa(1, i)] += 1;

    struct imm_abc const *abc = imm_super(amino);
    imm_float lprobs[255] = {0};
    for (unsigned i = 0; i < imm_abc_size(abc); ++i)
    {
        char aa = imm_abc_symbols(abc)[i];
        imm_float norm = imm_log((imm_float)count[(unsigned)aa]);
        lprobs[(unsigned)aa] = imm_amino_lprob_get(aminop, aa) - norm;
    }

    for (unsigned i = 0; i < imm_gc_size(); ++i)
    {
        char aa = imm_gc_aa(1, i);
        imm_codon_lprob_set(codonp, imm_gc_codon(1, i), lprobs[(unsigned)aa]);
    }
    return imm_codon_lprob_normalize(codonp);
}

int dcp_pp_add_node(struct dcp_pp *pp, imm_float lodds[static 1])
{
    struct imm_abc const *abc = imm_super(pp->nuclt);
    unsigned idx = pp->curr_node_idx;
    unsigned match_state_id = MATCH_ID | idx;
    unsigned insert_state_id = INSERT_ID | idx;
    unsigned delete_state_id = DELETE_ID | idx;
    imm_frame_state_new(match_state_id, &pp->nucltp, pp->alt.codonm,
                        pp->epsilon);
    imm_frame_state_new(insert_state_id, &pp->nucltp, pp->alt.codonm,
                        pp->epsilon);
    imm_mute_state_new(delete_state_id, abc);
}

void dcp_pp_destroy(struct dcp_pp *pp) { free(pp); }
