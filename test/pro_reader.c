#include "dcp/dcp.h"
#include "hope/hope.h"

static char const sequence[] =
    "GTGCTGGGCAGCAAAAGCCTGACCGCGAAAAGCCTGCTGGGCACCCTGGGCTTTCTGCAT"
    "ACCCAGCCGGATAACAGCGAAGGCAGCAAACCGGTGGGCGCGAAAGTGGCGCTGAGCACC"
    "AAAAGCGAAACCTGCATTGAAGAATGGGATCTGTATCGCCGCAGCAGCATTCTGGGCGGC"
    "GATAAAAAACTGGCGAAATTTCCGGAAATGGAAACCCCGGCGGAAGTGAAAAAAGTGCAG"
    "ACCATGCTGGCGGCGGGCGATGTGAGCCTGGCGGTGATGAAAGCGGCGCCGAAAGGCATT"
    "CCGTGCGCGAAAAGCTGCAACACCCGCGAAGTGGATGGCCGCCCGCTGAACCCGCTGCAG"
    "ATTGCGAACTATCTGGCGTATAAACTGAGCGCGGATTGCGCGGGCGCGTATGCGGCGGCG"
    "GATCTGGCGGAAACCGTGGGCAAAAAAACCGAAAACGTGCTGGGCCTGCATTATCATGAA"
    "GTGATGGCGCTGCTGCTGCTGGGCAGCAAAAGCCTGACCGCGAAAAGCCTGCTGGGCACC"
    "CTGGGCTTTCTGCATACCCAGCCGGATAACAGCGAAGGCAGCAAACCGGTGGGCGCGAAA"
    "GTGGCGCTGAGCACCAAAAGCGAAACCTGCATTGAAGAATGGGATCTGTATCGCCGCAGC"
    "AGCATTCTGGGCGGCGATAAAAAACTGGCGAAATTTCCGGAAATGGAAACCCCGGCGGAA"
    "GTGAAAAAAGTGCAGACCATGCTGGCGGCGGGCGATGTGAGCCTGGCGGTGATGAAAGCG"
    "GCGCCGAAAGGCATTCCGTGCGCGAAAAGCTGCAACACCCGCGAAGTGGATGGCCGCCCG"
    "CTGAACCCGCTGCAGATTGCGGATAAAAAACTGGCGAAATTTCCGGAAATGGAAACCCCG"
    "GCGGAAGTGAAAAAAGTGCAGACCATGCTGGCGGCGGGCGATGTGAGCCTGGCGGTGATG"
    "AAAGCGGCGCCGAAAGGCATTCCGTGCGCGAAAAGCTGCAACACCCGCGAAGTGGATGGC"
    "CGCCCGCTGAACCCGCTGCAGATTGCGTTTAGC";

int main(void)
{
    struct imm_amino const *amino = &imm_amino_iupac;
    struct imm_nuclt const *nuclt = imm_super(&imm_dna_iupac);
    struct imm_nuclt_code code;
    imm_nuclt_code_init(&code, nuclt);

    FILE *fd = fopen(ASSETS "/PF02545.hmm", "r");
    NOTNULL(fd);
    struct dcp_pro_cfg cfg = dcp_pro_cfg(DCP_ENTRY_DIST_OCCUPANCY, 0.01f);

    struct dcp_pro_reader reader;
    dcp_pro_reader_init(&reader, amino, &code, cfg, fd);

    EQ(dcp_pro_reader_next(&reader), DCP_DONE);

    struct dcp_pro_prof prof;
    dcp_pro_prof_init(&prof, amino, &code, cfg);

    dcp_prof_nameit(dcp_super(&prof), dcp_meta("name", "acc"));
    EQ(dcp_pro_prof_absorb(&prof, &reader.model), DCP_DONE);

    struct imm_seq seq =
        imm_seq(imm_str(sequence), dcp_super(&prof)->code->abc);

    dcp_pro_prof_setup(&prof, imm_seq_size(&seq), true, false);

    struct imm_prod prod = imm_prod();
    struct imm_dp *dp = &prof.alt.dp;
    struct imm_task *task = imm_task_new(dp);
    NOTNULL(task);
    EQ(imm_task_setup(task, &seq), IMM_SUCCESS);
    EQ(imm_dp_viterbi(dp, task, &prod), IMM_SUCCESS);
    CLOSE(prod.loglik, -1430.9281381240353);
    imm_del(&prod);

    fclose(fd);
    dcp_del(&prof);
    return hope_status();
}
