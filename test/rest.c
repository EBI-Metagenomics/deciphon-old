#include "rest.h"
#include "hope/hope.h"

struct rest_pend_job pend_job = {0};
struct sched_seq sched_seq = {0};

static void test_rest_next_pending_job(void)
{
    enum rc rc = rest_next_pend_job(&pend_job);
    EQ(rc, RC_DONE);
    printf("id: %lld db_id: %lld multi_hits: %d hmmer3_compat: %d\n",
           pend_job.id, pend_job.db_id, pend_job.multi_hits,
           pend_job.hmmer3_compat);
}

static void test_rest_next_seq(void)
{
    sched_seq.id = 0;
    sched_seq.job_id = 1;
    enum rc rc = rest_next_seq(&sched_seq);
    EQ(rc, RC_DONE);
    printf("id: %lld job_id: %lld name: %s\n", sched_seq.id, sched_seq.job_id,
           sched_seq.name);
    printf("data: %s\n", sched_seq.data);

    sched_seq.id = 1;
    rc = rest_next_seq(&sched_seq);
    EQ(rc, RC_DONE);
    printf("id: %lld job_id: %lld name: %s\n", sched_seq.id, sched_seq.job_id,
           sched_seq.name);
    printf("data: %s\n", sched_seq.data);
}

int main(void)
{
    test_rest_next_pending_job();
    test_rest_next_seq();
    return hope_status();
}
