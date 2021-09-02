#include "dcp/dcp.h"
#include "hope/hope.h"
#include "imm/imm.h"

void test_pro_reader(void);

int main(void)
{
    test_pro_reader();
    return hope_status();
}

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

void test_pro_reader(void)
{
    struct dcp_pro_reader reader;
    FILE *fd = fopen(ASSETS "/PF02545.hmm", "r");
    NOTNULL(fd);
    struct dcp_pro_cfg cfg = {&imm_amino_iupac, imm_super(imm_gc_dna()),
                              DCP_ENTRY_DIST_OCCUPANCY, 0.01f};
    dcp_pro_reader_init(&reader, cfg, fd);

    enum dcp_rc rc = dcp_pro_reader_next(&reader);
    EQ(rc, DCP_SUCCESS);

    struct dcp_pro_prof p;
    dcp_pro_prof_init(&p, cfg);

    dcp_prof_nameit(dcp_super(&p), dcp_meta("name", "acc"));
    EQ(dcp_pro_prof_absorb(&p, &reader.model), DCP_SUCCESS);

    struct imm_seq seq = imm_seq(imm_str(sequence), dcp_super(&p)->abc);

    dcp_pro_prof_setup(&p, imm_seq_size(&seq), true, false);

    struct imm_result result = imm_result();
    struct imm_dp *dp = &p.alt.dp;
    struct imm_task *task = imm_task_new(dp);
    NOTNULL(task);
    EQ(imm_task_setup(task, &seq), IMM_SUCCESS);
    EQ(imm_dp_viterbi(dp, task, &result), IMM_SUCCESS);

    CLOSE(result.loglik, -1430.9281381240353);

    fclose(fd);
}
