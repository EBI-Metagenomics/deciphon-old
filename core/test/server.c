#include "server.h"
#include "hope/hope.h"
#include "job.h"
#include "protein_db_examples.h"
#include "standard_db_examples.h"

/* void test_server_setup(unsigned num_threads); */
/* void test_server_reopen(void); */
/* void test_server_standard_db(void); */
/* void test_server_submit_standard_job(void); */
void test_server_submit_protein_job(void);
/* void test_server_submit_job_with_error(void); */
/* void test_server_submit_and_fetch_job(unsigned num_threads); */

int main(void)
{
    /* test_server_setup(1); */
    /* test_server_setup(2); */
    /* test_server_reopen(); */
    /* test_server_standard_db(); */
    /* test_server_submit_standard_job(); */
    test_server_submit_protein_job();
    /* test_server_submit_job_with_error(); */
    /* test_server_submit_and_fetch_job(1); */
    /* test_server_submit_and_fetch_job(4); */
    return hope_status();
}

#if 0
void test_server_setup(unsigned num_threads)
{
    remove(TMPDIR "/setup.sched");
    EQ(server_open(TMPDIR "/setup.sched", num_threads), RC_DONE);
    EQ(server_close(), RC_DONE);
}

void test_server_reopen(void)
{
    remove(TMPDIR "/reopen.sched");

    EQ(server_open(TMPDIR "/reopen.sched", 1), RC_DONE);
    EQ(server_close(), RC_DONE);

    EQ(server_open(TMPDIR "/reopen.sched", 1), RC_DONE);
    EQ(server_close(), RC_DONE);
}

void test_server_standard_db(void)
{
    char const db_path[] = TMPDIR "/standard_db.sched";
    char const ex_path[] = TMPDIR "/standard_example1.dcp";

    remove(db_path);

    EQ(server_open(db_path, 1), RC_DONE);

    standard_db_examples_new_ex1(ex_path);
    int64_t db_id = 0;
    EQ(server_add_db(ex_path, &db_id), RC_DONE);

    EQ(server_close(), RC_DONE);
}

void test_server_submit_standard_job(void)
{
    char const db_path[] = TMPDIR "/submit_job.sched";
    char const ex_path[] = TMPDIR "/submit_job.dcp";
    remove(db_path);

    EQ(server_open(db_path, 1), RC_DONE);

    standard_db_examples_new_ex1(ex_path, 2);
    int64_t db_id = 0;
    EQ(server_add_db(ex_path, &db_id), RC_DONE);
    EQ(db_id, 1);

    struct job job = {0};
    job_init(&job, db_id);
    struct seq seq = {0};
    seq_init(&seq, "seq0", imm_str("BMIME"));
    /* seq_init(&seq, "seq0", imm_str(imm_example1_seq)); */
    job_add_seq(&job, &seq);

    EQ(server_submit_job(&job), RC_DONE);
    EQ(job.id, 1);

    EQ(server_run(true), RC_DONE);

    EQ(server_close(), RC_DONE);
}
#endif

struct sched_job sched_job = {0};
struct sched_prod prod = {0};

void test_server_submit_protein_job(void)
{
    char const db_path[] = TMPDIR "/submit_protein_job.sched";
    char const ex_path[] = TMPDIR "/submit_protein_job.dcp";
    remove(db_path);

    EQ(server_open(db_path, 1), RC_DONE);

    protein_db_examples_new_ex1(ex_path, 2);
    int64_t db_id = 0;
    EQ(server_add_db(ex_path, &db_id), RC_DONE);
    EQ(db_id, 1);

    struct job job = {0};
    job_init(&job, db_id);

    struct seq seq[4 * 4 * 4] = {0};
    char letters[] = "ACGT";
    char strs[4 * 4 * 4][6];
    unsigned k = 0;
    server_set_lrt_threshold(7.f);
    for (unsigned i0 = 0; i0 < 4; ++i0)
    {
        for (unsigned i1 = 0; i1 < 4; ++i1)
        {
            for (unsigned i2 = 0; i2 < 4; ++i2)
            {
                char name[8] = {0};
                sprintf(name, "%d", k);
                name[0] = 's';
                name[1] = 'e';
                name[2] = 'q';
                name[4] = '0';
                strs[k][0] = letters[i0];
                strs[k][1] = letters[i1];
                strs[k][2] = letters[i2];
                strs[k][3] = letters[i0];
                strs[k][4] = letters[i1];
                strs[k][5] = 0;
                seq_init(seq + k, name, imm_str(strs[k]));
                job_add_seq(&job, seq + k);
                ++k;
            }
        }
    }

    EQ(server_submit_job(&job), RC_DONE);
    EQ(job.id, 1);

    EQ(server_run(true), RC_DONE);

    sched_job.id = job.id;
    EQ(server_get_sched_job(&sched_job), RC_DONE);
    EQ(sched_job.error, "");
    EQ(sched_job.multi_hits, true);
    EQ(sched_job.hmmer3_compat, false);
    EQ(sched_job.state, "done");

    prod.id = 0;
    prod.job_id = job.id;
    while (server_next_sched_prod(&sched_job, &prod) == RC_NEXT)
    {
        EQ(prod.abc_name, "dna");
        EQ(prod.job_id, job.id);
        if (prod.seq_id == 4)
        {
            EQ(prod.profile_name, "ACC1");
            CLOSE(prod.alt_loglik, -9.380424500);
            CLOSE(prod.null_loglik, -13.256095886);
        }
        else if (prod.seq_id == 38)
        {
            EQ(prod.profile_name, "ACC0");
            CLOSE(prod.alt_loglik, -9.601443291);
            CLOSE(prod.null_loglik, -13.496196747);
        }
        else if (prod.seq_id == 39)
        {
            EQ(prod.profile_name, "ACC0");
            CLOSE(prod.alt_loglik, -9.601497650);
            CLOSE(prod.null_loglik, -13.305152893);
        }
    }

    EQ(server_close(), RC_DONE);
}

#if 0
void test_server_submit_job_with_error(void)
{
    char const db_path[] = TMPDIR "/submit_job_with_error.sched";
    char const ex_path[] = TMPDIR "/standard_example1.dcp";
    remove(db_path);

    EQ(server_open(db_path, 1), RC_DONE);

    standard_db_examples_new_ex1(ex_path, 2);
    int64_t db_id = 0;
    EQ(server_add_db(ex_path, &db_id), RC_DONE);
    EQ(db_id, 1);

    struct job job = {0};
    job_init(&job, db_id);
    struct seq seq[2] = {0};
    seq_init(seq + 0, "seq0", imm_str(imm_example1_seq));
    seq_init(seq + 1, "seq1", imm_str(imm_example2_seq));
    job_add_seq(&job, seq + 0);
    job_add_seq(&job, seq + 1);

    EQ(server_submit_job(&job), RC_DONE);
    EQ(job.id, 1);

    EQ(server_run(true), RC_DONE);

    EQ(server_close(), RC_DONE);
}

void test_server_submit_and_fetch_job(unsigned num_threads)
{
    char const db_path[] = TMPDIR "/submit_and_fetch_job.sched";
    char const ex_path[] = TMPDIR "/protein_example1.dcp";
    remove(db_path);

    EQ(server_open(db_path, num_threads), RC_DONE);

    protein_db_examples_new_ex1(ex_path, 2);
    int64_t db_id = 0;
    EQ(server_add_db(ex_path, &db_id), RC_DONE);
    EQ(db_id, 1);

    struct job job = {0};
    job_init(&job, db_id);
    struct seq seq[2] = {0};
    seq_init(seq + 0, "seq0", imm_str(imm_example1_seq));
    seq_init(seq + 1, "seq1", imm_str(imm_example2_seq));
    job_add_seq(&job, seq + 0);
    job_add_seq(&job, seq + 1);

    EQ(server_submit_job(&job), RC_DONE);
    EQ(job.id, 1);

    enum job_state state = 0;
    EQ(server_job_state(job.id, &state), RC_DONE);
    EQ(state, JOB_PEND);

    EQ(server_job_state(2, &state), RC_NOTFOUND);

    EQ(server_run(true), RC_DONE);

    int64_t prod_id = 0;
    EQ(server_next_prod(1, &prod_id), RC_NEXT);
    EQ(prod_id, 1);

    struct prod const *p = server_get_prod();

    EQ(p->id, 1);
    EQ(p->seq_id, 2);
    EQ(p->match_id, 1);
    EQ(p->prof_name, "ACC0");
    EQ(p->abc_name, "dna_iupac");
    CLOSE(p->loglik, -2720.38134765625);
    CLOSE(p->null_loglik, -3163.185302734375);
    EQ(p->prof_typeid, "protein");
    EQ(p->version, "0.0.4");

    extern char const prod1_match_data[];
    for (unsigned i = 0; i < array_size(p->match); ++i)
    {
        EQ(array_data(p->match)[i], prod1_match_data[i]);
    }

    EQ(server_next_prod(1, &prod_id), RC_NEXT);
    EQ(prod_id, 2);

    EQ(p->id, 2);
    EQ(p->seq_id, 2);
    EQ(p->match_id, 2);
    EQ(p->prof_name, "ACC1");
    EQ(p->abc_name, "dna_iupac");
    CLOSE(p->loglik, -2854.53369140625);
    CLOSE(p->null_loglik, -3094.66308593750);
    EQ(p->prof_typeid, "protein");
    EQ(p->version, "0.0.4");

    EQ(server_next_prod(1, &prod_id), RC_DONE);

    EQ(server_close(), RC_DONE);
}
#endif
