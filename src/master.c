#include "deciphon/deciphon.h"
#include "lib/c-list.h"
#include "lib/c11threads.h"
#include "nmm/nmm.h"
#ifdef _OPENMP
#include <omp.h>
#endif

void process(char const* seq_str, struct nmm_profile const* prof);

struct task
{
    struct nmm_profile const* profile;
    struct CList              link;
};

struct task* task_create(struct nmm_profile const* prof);

struct task* task_create(struct nmm_profile const* prof)
{
    struct task* task = malloc(sizeof(*task));
    task->profile = prof;
    c_list_init(&task->link);
    return task;
}

struct queue
{
    struct CList tasks;
    uint32_t     ntasks;
    uint32_t     max_size;

    mtx_t mtx;
    cnd_t empty;
    cnd_t full;
    bool  finished;
};

struct queue* queue_create(uint32_t max_size);
void          queue_push(struct queue* queue, struct task* task);
struct task*  queue_pop(struct queue* queue);
uint32_t      queue_size(struct queue* queue);
void          queue_finish(struct queue* queue);

struct queue* queue_create(uint32_t max_size)
{
    struct queue* queue = malloc(sizeof(*queue));

    c_list_init(&queue->tasks);
    queue->ntasks = 0;
    queue->max_size = max_size;

    mtx_init(&queue->mtx, mtx_plain);
    cnd_init(&queue->empty);
    cnd_init(&queue->full);
    queue->finished = false;

    return queue;
}

void queue_push(struct queue* queue, struct task* task)
{
    mtx_lock(&queue->mtx);

    while (queue->ntasks >= queue->max_size) {
        struct timespec now;
        timespec_get(&now, TIME_UTC);
        now.tv_sec += 1;
        cnd_timedwait(&queue->full, &queue->mtx, &now);
    }

    c_list_link_tail(&queue->tasks, &task->link);
    queue->ntasks++;

    if (queue->ntasks == 1)
        cnd_broadcast(&queue->empty);

    mtx_unlock(&queue->mtx);
}

struct task* queue_pop(struct queue* queue)
{
    mtx_lock(&queue->mtx);

    while (queue->ntasks == 0 && !queue->finished) {
        cnd_wait(&queue->empty, &queue->mtx);
    }

    if (queue->ntasks == 0 && queue->finished) {
        mtx_unlock(&queue->mtx);
        return NULL;
    }

    struct CList* elem = c_list_first(&queue->tasks);
    struct task*  task = NULL;
    if (elem) {
        task = c_list_entry(elem, struct task, link);
        c_list_unlink(elem);
        queue->ntasks--;
        if (queue->ntasks + 1 == queue->max_size)
            cnd_broadcast(&queue->full);
    }

    mtx_unlock(&queue->mtx);
    return task;
}

uint32_t queue_size(struct queue* queue) { return queue->ntasks; }

void queue_finish(struct queue* queue)
{
    mtx_lock(&queue->mtx);
    queue->finished = true;
    mtx_unlock(&queue->mtx);
}

int dcp_master(char const* db_filepath, char const* seq_str)
{
    struct dcp_input*     input = dcp_input_create(db_filepath);
    struct dcp_partition* part = dcp_input_create_partition(input, 0, 1);
    struct queue*         queue = queue_create(512);

#pragma omp parallel default(none) shared(part, seq_str, queue) /* if(1 == 2) */
    {
#pragma omp master
        {
            int i = 0;
            while (!dcp_partition_end(part)) {
                printf("Producer %d\n", i++);
                struct nmm_profile const* prof = dcp_partition_read(part);
                struct task*              task = task_create(prof);
                queue_push(queue, task);
            }
            queue_finish(queue);
        }

        while (true) {
            struct task* task = NULL;

            task = queue_pop(queue);
            if (task) {
                printf("Process\n");
                /* process(seq_str, task->profile); */
            } else {
                break;
            }
        }
    }
    dcp_partition_destroy(part);
    dcp_input_destroy(input);
    return 0;
}

void process(char const* seq_str, struct nmm_profile const* prof)
{
    struct imm_abc const* abc = nmm_profile_abc(prof);
    struct imm_seq const* seq = imm_seq_create(seq_str, abc);

    struct imm_model* alt = nmm_profile_get_model(prof, 0);
    /* struct imm_model*    null = nmm_profile_get_model(prof, 1); */
    struct imm_hmm*      hmm_alt = imm_model_hmm(alt);
    struct imm_dp const* dp_alt = imm_model_dp(alt);
    /* struct imm_hmm*      hmm_null = imm_model_hmm(null); */
    /* struct imm_dp const* dp_null = imm_model_dp(null); */

    struct imm_dp_task* task_alt = imm_dp_task_create(dp_alt);
    imm_dp_task_setup(task_alt, seq, 0);
    struct imm_results const* results = imm_dp_viterbi(dp_alt, task_alt);
    /* printf("#Results: %d\n", imm_results_size(results)); */
    struct imm_result const* r = imm_results_get(results, 0);
    struct imm_subseq        subseq = imm_result_subseq(r);
    imm_float                loglik = imm_hmm_loglikelihood(hmm_alt, imm_subseq_cast(&subseq), imm_result_path(r));
    printf("loglik: %f\n", loglik);

    nmm_profile_destroy(prof);
}
