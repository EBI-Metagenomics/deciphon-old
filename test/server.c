#include "dcp/dcp.h"
#include "hope/hope.h"
#include "std_db_examples.h"

void test_server_setup(void);
void test_server_reopen(void);
void test_server_std_db(void);
void test_server_submit_job(void);

int main(void)
{
    test_server_setup();
    test_server_reopen();
    test_server_std_db();
    test_server_submit_job();
    return hope_status();
}

void test_server_setup(void)
{
    remove(TMPDIR "/setup.sqlite3");
    struct dcp_server *srv = dcp_server_open(TMPDIR "/setup.sqlite3");
    NOTNULL(srv);
    EQ(dcp_server_close(srv), DCP_SUCCESS);
}

void test_server_reopen(void)
{
    remove(TMPDIR "/reopen.sqlite3");

    struct dcp_server *srv = dcp_server_open(TMPDIR "/reopen.sqlite3");
    NOTNULL(srv);
    EQ(dcp_server_close(srv), DCP_SUCCESS);

    srv = dcp_server_open(TMPDIR "/reopen.sqlite3");
    NOTNULL(srv);
    EQ(dcp_server_close(srv), DCP_SUCCESS);
}

void test_server_std_db(void)
{
    remove(TMPDIR "/add_std_db.sqlite3");

    struct dcp_server *srv = dcp_server_open(TMPDIR "/std_db.sqlite3");
    NOTNULL(srv);

    std_db_examples_new_ex1(TMPDIR "/example1.dcp");
    EQ(dcp_server_add_db(srv, TMPDIR "/example1.dcp"), DCP_SUCCESS);

    EQ(dcp_server_close(srv), DCP_SUCCESS);
}

void test_server_submit_job(void)
{
    remove(TMPDIR "/submit_job.sqlite3");

    struct dcp_server *srv = dcp_server_open(TMPDIR "/submit_job.sqlite3");
    NOTNULL(srv);

    std_db_examples_new_ex1(TMPDIR "/example1.dcp");
    EQ(dcp_server_add_db(srv, TMPDIR "/example1.dcp"), DCP_SUCCESS);
    /* sqlite3_last_insert_rowid */

#if 0
    sched_job_add_seq;
    struct imm_nuclt const *nuclt = imm_super(&imm_dna_iupac);

    struct dcp_abc abc;
    dcp_abc_init(&abc, IMM_DNA, imm_super(nuclt));

    struct dcp_job job;
    dcp_job_init(&job, &abc);

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
#endif
}
