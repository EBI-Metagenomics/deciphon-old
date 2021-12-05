#include "job.h"
#include "hope/hope.h"
#include "rc.h"

void test_job(void);

int main(void)
{
    test_job();
    return hope_status();
}

void test_job(void)
{
    struct job job;
    job_init(&job, 1);

    char const *str[2] = {"ATGAAACGCATTAGCACCACCATTACCACCAC",
                          "AAACGCATTAGCACCACCA"};
    struct imm_str istr[2] = {IMM_STR(str[0]), IMM_STR(str[1])};

    struct seq seqs[2];
    seq_init(&seqs[0], "seq0", istr[0]);
    seq_init(&seqs[1], "seq1", istr[1]);

    job_add_seq(&job, &seqs[0]);
    job_add_seq(&job, &seqs[1]);
}
