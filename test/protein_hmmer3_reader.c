#include "protein_hmmer3_reader.h"
#include "hope/hope.h"
#include "imm/imm.h"
#include "protein_profile.h"

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
    struct protein_cfg cfg = protein_cfg(ENTRY_DIST_OCCUPANCY, 0.01f);

    struct protein_hmmer3_reader reader;
    protein_hmmer3_reader_init(&reader, amino, &code, cfg, fd);

    EQ(protein_hmmer3_reader_next(&reader), DCP_OK);

    struct protein_profile prof;
    protein_profile_init(&prof, amino, &code, cfg);

    EQ(protein_profile_absorb(&prof, &reader.model), DCP_OK);

    struct imm_seq seq = imm_seq(imm_str(sequence), prof.super.code->abc);

    protein_profile_setup(&prof, imm_seq_size(&seq), true, false);

    struct imm_prod prod = imm_prod();
    struct imm_dp *dp = &prof.alt.dp;
    struct imm_task *task = imm_task_new(dp);
    NOTNULL(task);
    EQ(imm_task_setup(task, &seq), IMM_SUCCESS);
    EQ(imm_dp_viterbi(dp, task, &prod), IMM_SUCCESS);
    CLOSE(prod.loglik, -1430.9281381240353);
    imm_del(&prod);

    fclose(fd);
    imm_task_del(task);
    profile_del((struct profile *)&prof);
    protein_hmmer3_reader_del(&reader);
    return hope_status();
}
