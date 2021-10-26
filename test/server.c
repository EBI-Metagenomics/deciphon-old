#include "dcp/dcp.h"
#include "hope/hope.h"
#include "std_db_examples.h"

void test_server_setup(void);
void test_server_reopen(void);
void test_server_add_std_db(void);

int main(void)
{
    test_server_setup();
    test_server_reopen();
    test_server_add_std_db();
    return hope_status();
}

void test_server_setup(void)
{
    struct dcp_server srv = DCP_SERVER_INIT();
    remove(TMPDIR "/setup.sqlite3");
    EQ(dcp_server_setup(&srv, TMPDIR "/setup.sqlite3"), DCP_SUCCESS);
    EQ(dcp_server_close(&srv), DCP_SUCCESS);
}

void test_server_reopen(void)
{
    struct dcp_server srv = DCP_SERVER_INIT();
    remove(TMPDIR "/reopen.sqlite3");

    EQ(dcp_server_setup(&srv, TMPDIR "/reopen.sqlite3"), DCP_SUCCESS);
    EQ(dcp_server_close(&srv), DCP_SUCCESS);

    EQ(dcp_server_setup(&srv, TMPDIR "/reopen.sqlite3"), DCP_SUCCESS);
    EQ(dcp_server_close(&srv), DCP_SUCCESS);
}

void test_server_add_std_db(void)
{
    struct dcp_server srv = DCP_SERVER_INIT();

    remove(TMPDIR "/add_std_db.sqlite3");
    EQ(dcp_server_setup(&srv, TMPDIR "/add_std_db.sqlite3"), DCP_SUCCESS);

    std_db_examples_new_ex1(TMPDIR "/example1.dcp");
    EQ(dcp_server_add_db(&srv, 1, "example1", TMPDIR "/example1.dcp"),
       DCP_SUCCESS);

    struct imm_nuclt const *nuclt = imm_super(&imm_dna_iupac);

    struct dcp_job job;
    dcp_job_init(&job, imm_super(nuclt));

    char const *str[2] = {"ATGAAACGCATTAGCACCACCATTACCACCAC",
                          "AAACGCATTAGCACCACCA"};
    struct imm_seq seq[2] = {imm_seq(imm_str(str[0]), imm_super(nuclt)),
                             imm_seq(imm_str(str[1]), imm_super(nuclt))};

    struct dcp_seq seqs[2];
    dcp_seq_init(&seqs[0], seq[0]);
    dcp_seq_init(&seqs[1], seq[1]);

    EQ(dcp_job_add(&job, &seqs[0]), DCP_SUCCESS);
    EQ(dcp_job_add(&job, &seqs[1]), DCP_SUCCESS);

    job.cfg.hmmer3_compat = false;
    job.cfg.multiple_hits = true;

    /* dcp_server_add_job(&srv, &job); */
    /* /Users/horta/data/Pfam-A.5.dcp */

    EQ(dcp_server_close(&srv), DCP_SUCCESS);
}
