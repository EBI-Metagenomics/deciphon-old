#include "dcp/pro/reader.h"
#include "dcp.h"
#include "dcp/rc.h"

static void init_null_lprobs(imm_float[IMM_AMINO_SIZE]);

void dcp_pro_reader_init(struct dcp_pro_reader *reader, struct dcp_pro_cfg cfg,
                         FILE *restrict fd)
{
    reader->cfg = cfg;
    hmr_init(&reader->hmr, fd);
    hmr_prof_init(&reader->prof, &reader->hmr);

    init_null_lprobs(reader->null_lprobs);
    for (unsigned i = 0; i < ARRAY_SIZE(reader->null_lodds); ++i)
        reader->null_lodds[i] = 0.0f;
}

enum dcp_rc dcp_pro_reader_next(struct dcp_pro_reader *reader)
{
    enum hmr_rc hmr_rc = hmr_next_prof(&reader->hmr, &reader->prof);
    if (hmr_rc == HMR_ENDFILE) return DCP_ENDFILE;

    if (hmr_rc) return DCP_RUNTIMEERROR;

    enum dcp_rc rc = dcp_pro_model_init(
        &reader->model, reader->cfg, reader->null_lprobs, reader->null_lodds);

    if (rc) return rc;

    unsigned core_size = hmr_prof_length(&reader->prof);
    if ((rc = dcp_pro_model_setup(&reader->model, core_size))) return rc;

    hmr_rc = hmr_next_node(&reader->hmr, &reader->prof);
    assert(hmr_rc != HMR_ENDFILE);

    struct dcp_pro_trans t = {
        .MM = (imm_float)reader->prof.node.trans[HMR_TRANS_MM],
        .MI = (imm_float)reader->prof.node.trans[HMR_TRANS_MI],
        .MD = (imm_float)reader->prof.node.trans[HMR_TRANS_MD],
        .IM = (imm_float)reader->prof.node.trans[HMR_TRANS_IM],
        .II = (imm_float)reader->prof.node.trans[HMR_TRANS_II],
        .DM = (imm_float)reader->prof.node.trans[HMR_TRANS_DM],
        .DD = (imm_float)reader->prof.node.trans[HMR_TRANS_DD],
    };
    rc = dcp_pro_model_add_trans(&reader->model, t);
    assert(!rc);

    while (!(hmr_rc = hmr_next_node(&reader->hmr, &reader->prof)))
    {
        imm_float match_lprobs[IMM_AMINO_SIZE];
        for (unsigned i = 0; i < IMM_AMINO_SIZE; ++i)
            match_lprobs[i] = (imm_float)reader->prof.node.match[i];

        rc = dcp_pro_model_add_node(&reader->model, match_lprobs);
        assert(!rc);

        struct dcp_pro_trans t2 = {
            .MM = (imm_float)reader->prof.node.trans[HMR_TRANS_MM],
            .MI = (imm_float)reader->prof.node.trans[HMR_TRANS_MI],
            .MD = (imm_float)reader->prof.node.trans[HMR_TRANS_MD],
            .IM = (imm_float)reader->prof.node.trans[HMR_TRANS_IM],
            .II = (imm_float)reader->prof.node.trans[HMR_TRANS_II],
            .DM = (imm_float)reader->prof.node.trans[HMR_TRANS_DM],
            .DD = (imm_float)reader->prof.node.trans[HMR_TRANS_DD],
        };
        rc = dcp_pro_model_add_trans(&reader->model, t2);
        assert(!rc);
    }
    assert(hmr_rc == HMR_ENDNODE);

    return DCP_SUCCESS;
}

static void init_null_lprobs(imm_float lprobs[IMM_AMINO_SIZE])
{
    /* Copy/paste from HMMER3 amino acid frequences inferred from Swiss-Prot
     * 50.8, (Oct 2006), counting over 85956127 (86.0M) residues. */
    *(lprobs++) = imm_log(0.0787945); /*"A":*/
    *(lprobs++) = imm_log(0.0151600); /*"C":*/
    *(lprobs++) = imm_log(0.0535222); /*"D":*/
    *(lprobs++) = imm_log(0.0668298); /*"E":*/
    *(lprobs++) = imm_log(0.0397062); /*"F":*/
    *(lprobs++) = imm_log(0.0695071); /*"G":*/
    *(lprobs++) = imm_log(0.0229198); /*"H":*/
    *(lprobs++) = imm_log(0.0590092); /*"I":*/
    *(lprobs++) = imm_log(0.0594422); /*"K":*/
    *(lprobs++) = imm_log(0.0963728); /*"L":*/
    *(lprobs++) = imm_log(0.0237718); /*"M":*/
    *(lprobs++) = imm_log(0.0414386); /*"N":*/
    *(lprobs++) = imm_log(0.0482904); /*"P":*/
    *(lprobs++) = imm_log(0.0395639); /*"Q":*/
    *(lprobs++) = imm_log(0.0540978); /*"R":*/
    *(lprobs++) = imm_log(0.0683364); /*"S":*/
    *(lprobs++) = imm_log(0.0540687); /*"T":*/
    *(lprobs++) = imm_log(0.0673417); /*"V":*/
    *(lprobs++) = imm_log(0.0114135); /*"W":*/
    *(lprobs++) = imm_log(0.0304133); /*"Y":*/
};
