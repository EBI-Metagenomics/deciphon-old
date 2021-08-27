#include "dcp/pro/reader.h"
#include "support.h"

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

enum dcp_rc dcp_pro_reader_next(struct dcp_pro_reader *reader,
                                struct dcp_pro_model *model)
{
    enum hmr_rc hmr_rc = hmr_next_prof(&reader->hmr, &reader->prof);
    if (hmr_rc == HMR_ENDFILE)
        return DCP_ENDFILE;

    if (hmr_rc)
        return DCP_RUNTIMERROR;

    enum dcp_rc rc = dcp_pro_model_init(
        &reader->model, reader->cfg, reader->null_lprobs, reader->null_lodds);

    if (rc)
        return rc;

    unsigned core_size = hmr_prof_length(&reader->prof);
    if ((rc = dcp_pro_model_setup(&reader->model, core_size)))
        return rc;

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
}
