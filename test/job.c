#include "dcp/dcp.h"
#include "hope/hope.h"

void test_job(void);
void test_job_wrong_abc(void);

int main(void)
{
    test_job();
    test_job_wrong_abc();
    return hope_status();
}

void test_job(void)
{
    struct imm_nuclt const *nuclt = imm_super(&imm_dna_iupac);

    struct dcp_abc abc;
    dcp_abc_init(&abc, IMM_DNA, imm_super(nuclt));

    struct dcp_job job;
    dcp_job_init(&job, &abc);

    char const *str[2] = {"ATGAAACGCATTAGCACCACCATTACCACCAC",
                          "AAACGCATTAGCACCACCA"};
    struct imm_seq seq[2] = {imm_seq(IMM_STR(str[0]), imm_super(nuclt)),
                             imm_seq(IMM_STR(str[1]), imm_super(nuclt))};

    struct dcp_seq seqs[2];
    dcp_seq_init(&seqs[0], seq[0]);
    dcp_seq_init(&seqs[1], seq[1]);

    EQ(dcp_job_add(&job, &seqs[0]), DCP_DONE);
    EQ(dcp_job_add(&job, &seqs[1]), DCP_DONE);
}

void test_job_wrong_abc(void)
{
    struct imm_nuclt const *nuclt0 = imm_super(&imm_dna_iupac);
    struct imm_nuclt const *nuclt1 = imm_super(&imm_rna_iupac);

    struct dcp_abc abc;
    dcp_abc_init(&abc, IMM_DNA, imm_super(nuclt0));

    struct dcp_job job;
    dcp_job_init(&job, &abc);

    char const str[] = "A";
    struct imm_seq seq = imm_seq(IMM_STR(str), imm_super(nuclt1));

    struct dcp_seq seqs;
    dcp_seq_init(&seqs, seq);

    EQ(dcp_job_add(&job, &seqs), DCP_ILLEGALARG);
}
