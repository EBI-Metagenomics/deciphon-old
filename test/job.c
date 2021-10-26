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

    struct dcp_job job;
    dcp_job_init(&job, imm_super(nuclt));

    char const *str[2] = {"ATGAAACGCATTAGCACCACCATTACCACCAC",
                          "AAACGCATTAGCACCACCA"};
    struct imm_seq seq[2] = {imm_seq(imm_str(str[0]), imm_super(nuclt)),
                             imm_seq(imm_str(str[1]), imm_super(nuclt))};

    struct dcp_target tgt[2];
    dcp_target_init(&tgt[0], seq[0]);
    dcp_target_init(&tgt[1], seq[1]);

    EQ(dcp_job_add(&job, &tgt[0]), DCP_SUCCESS);
    EQ(dcp_job_add(&job, &tgt[1]), DCP_SUCCESS);
}

void test_job_wrong_abc(void)
{
    struct imm_nuclt const *nuclt0 = imm_super(&imm_dna_iupac);
    struct imm_nuclt const *nuclt1 = imm_super(&imm_rna_iupac);

    struct dcp_job job;
    dcp_job_init(&job, imm_super(nuclt0));

    char const str[] = "A";
    struct imm_seq seq = imm_seq(imm_str(str), imm_super(nuclt1));

    struct dcp_target tgt;
    dcp_target_init(&tgt, seq);

    EQ(dcp_job_add(&job, &tgt), DCP_ILLEGALARG);
}
