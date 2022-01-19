#include "seq_queue.h"
#include "common/limits.h"
#include "seq.h"

struct seq_queue
{
    unsigned curr;
    struct sched_seq seq[NUM_SEQS_PER_JOB];
} queue = {0};

void seq_queue_init(void) { queue.curr = 0; }

void seq_queue_add(int64_t job_id, char const *name, char const *data)
{
    sched_seq_init(queue.seq + queue.curr++, job_id, name, data);
}

unsigned seq_queue_size(void) { return queue.curr; }

struct sched_seq *seq_queue_get(unsigned i) { return queue.seq + i; }