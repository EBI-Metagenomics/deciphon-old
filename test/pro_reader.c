#include "dcp/dcp.h"
#include "hope/hope.h"
#include "imm/imm.h"

#define FILEPATH1 "/Users/horta/data/PF02545_cut.hmm"
#define HMM_DOT1 "/Users/horta/tmp/PF02545_cut_hmm.dot"
#define DP_DOT1 "/Users/horta/tmp/PF02545_cut_dp.dot"

#define FILEPATH2 "/Users/horta/data/PF02545.hmm"
#define HMM_DOT2 "/Users/horta/tmp/PF02545_hmm.dot"
#define DP_DOT2 "/Users/horta/tmp/PF02545_dp.dot"

#define FILEPATH3 "/Users/horta/data/PF02545.hmm"

void test_pro_reader0(void);
void test_pro_reader1(void);
void test_pro_reader2(void);
void test_pro_reader3(void);

int main(void)
{
    /* test_pro_reader0(); */
    test_pro_reader1();
    test_pro_reader2();
    test_pro_reader3();
    return hope_status();
}

#if 0
void test_pro_reader0(void)
{
    struct dcp_pro_reader reader;
    FILE *fd = fopen(FILEPATH, "r");
    NOTNULL(fd);
    struct dcp_pro_cfg cfg = {&imm_amino_iupac, imm_super(imm_gc_dna()),
                              DCP_ENTRY_DIST_OCCUPANCY, 0.01f};
    dcp_pro_reader_init(&reader, cfg, fd);

    enum dcp_rc rc = dcp_pro_reader_next(&reader);
    EQ(rc, DCP_SUCCESS);

    FILE *dot_fd = fopen(OUTPUT, "w");
    dcp_pro_model_write_dot(&reader.model, dot_fd);
    fclose(dot_fd);

    struct dcp_pro_profile p;
    dcp_pro_profile_init(&p, cfg);

    dcp_profile_nameit(dcp_super(&p), dcp_meta("name", "acc"));
    EQ(dcp_pro_profile_absorb(&p, &reader.model), DCP_SUCCESS);

    /* char const str[] = "CCTGGTAAAGAAGATAATAACAAA"; */
    char const str[] = "CCTGGTAAAGAAGATAATAACAAA";
    struct imm_seq seq = imm_seq(imm_str(str), dcp_super(&p)->abc);

    dcp_pro_profile_setup(&p, imm_seq_size(&seq), true, false);

    struct imm_result result = imm_result();
    struct imm_dp *dp = &p.alt.dp;
    struct imm_task *task = imm_task_new(dp);
    imm_task_setup(task, &seq);
    imm_dp_viterbi(dp, task, &result);

    CLOSE(result.loglik, -37.60375230420982);

    dcp_del(&p);

    fclose(fd);
}
#endif

void test_pro_reader1(void)
{
    struct dcp_pro_reader reader;
    FILE *fd = fopen(FILEPATH1, "r");
    NOTNULL(fd);
    struct dcp_pro_cfg cfg = {&imm_amino_iupac, imm_super(imm_gc_dna()),
                              DCP_ENTRY_DIST_OCCUPANCY, 0.01f};
    dcp_pro_reader_init(&reader, cfg, fd);

    enum dcp_rc rc = dcp_pro_reader_next(&reader);
    EQ(rc, DCP_SUCCESS);

    FILE *hmm_dot_fd = fopen(HMM_DOT1, "w");
    dcp_pro_model_write_dot(&reader.model, hmm_dot_fd);
    fclose(hmm_dot_fd);

    struct dcp_pro_profile p;
    dcp_pro_profile_init(&p, cfg);

    dcp_profile_nameit(dcp_super(&p), dcp_meta("name", "acc"));
    dcp_pro_profile_absorb(&p, &reader.model);
    /* EQ(dcp_pro_profile_absorb(&p, &reader.model), DCP_SUCCESS); */

    char const str[] = "CCTGGTAAA";
    struct imm_seq seq = imm_seq(imm_str(str), dcp_super(&p)->abc);

    dcp_pro_profile_setup(&p, imm_seq_size(&seq), true, false);

    FILE *dp_dot_fd = fopen(DP_DOT1, "w");
    dcp_pro_profile_write_dot(&p, dp_dot_fd);
    fclose(dp_dot_fd);

    fclose(fd);
}

void test_pro_reader2(void)
{
    struct dcp_pro_reader reader;
    FILE *fd = fopen(FILEPATH2, "r");
    NOTNULL(fd);
    struct dcp_pro_cfg cfg = {&imm_amino_iupac, imm_super(imm_gc_dna()),
                              DCP_ENTRY_DIST_OCCUPANCY, 0.01f};
    dcp_pro_reader_init(&reader, cfg, fd);

    enum dcp_rc rc = dcp_pro_reader_next(&reader);
    EQ(rc, DCP_SUCCESS);

    FILE *hmm_dot_fd = fopen(HMM_DOT2, "w");
    dcp_pro_model_write_dot(&reader.model, hmm_dot_fd);
    fclose(hmm_dot_fd);

    struct dcp_pro_profile p;
    dcp_pro_profile_init(&p, cfg);

    dcp_profile_nameit(dcp_super(&p), dcp_meta("name", "acc"));
    dcp_pro_profile_absorb(&p, &reader.model);
    /* EQ(dcp_pro_profile_absorb(&p, &reader.model), DCP_SUCCESS); */

    char const str[] = "CCTGGTAAA";
    struct imm_seq seq = imm_seq(imm_str(str), dcp_super(&p)->abc);

    dcp_pro_profile_setup(&p, imm_seq_size(&seq), true, false);

    FILE *dp_dot_fd = fopen(DP_DOT2, "w");
    dcp_pro_profile_write_dot(&p, dp_dot_fd);
    fclose(dp_dot_fd);

    struct imm_result result = imm_result();
    struct imm_dp *dp = &p.alt.dp;
    struct imm_task *task = imm_task_new(dp);
    imm_task_setup(task, &seq);
    imm_dp_viterbi(dp, task, &result);

    CLOSE(result.loglik, -21.98851860153052);

    fclose(fd);
}

void test_pro_reader3(void)
{
    struct dcp_pro_reader reader;
    FILE *fd = fopen(FILEPATH3, "r");
    NOTNULL(fd);
    struct dcp_pro_cfg cfg = {&imm_amino_iupac, imm_super(imm_gc_dna()),
                              DCP_ENTRY_DIST_OCCUPANCY, 0.01f};
    dcp_pro_reader_init(&reader, cfg, fd);

    enum dcp_rc rc = dcp_pro_reader_next(&reader);
    EQ(rc, DCP_SUCCESS);

    struct dcp_pro_profile p;
    dcp_pro_profile_init(&p, cfg);

    dcp_profile_nameit(dcp_super(&p), dcp_meta("name", "acc"));
    EQ(dcp_pro_profile_absorb(&p, &reader.model), DCP_SUCCESS);

    char const str[] =
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
    struct imm_seq seq = imm_seq(imm_str(str), dcp_super(&p)->abc);

    dcp_pro_profile_setup(&p, imm_seq_size(&seq), true, false);

    struct imm_result result = imm_result();
    struct imm_dp *dp = &p.alt.dp;
    struct imm_task *task = imm_task_new(dp);
    imm_task_setup(task, &seq);
    imm_dp_viterbi(dp, task, &result);

    CLOSE(result.loglik, -1430.9281381240353);

    fclose(fd);
}
