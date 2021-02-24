#include "deciphon/deciphon.h"
#include "elapsed/elapsed.h"
#include "lib/c-list.h"
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
    /* omp_lock_t   lock; */
    struct CList tasks;
    uint32_t     ntasks;
    uint32_t     max_size;
    /* omp_lock_t   full; */
    /* omp_lock_t   empty; */
};

struct queue* queue_create(uint32_t max_size);
void          queue_push(struct queue* queue, struct task* task);
struct task*  queue_pop(struct queue* queue);
uint32_t      queue_size(struct queue* queue);
void          queue_lock_push(struct queue* queue);
void          queue_unlock_push(struct queue* queue);
/* void          queue_wait_not_full(struct queue* queue); */
/* void          queue_wait_not_empty(struct queue* queue); */

struct queue* queue_create(uint32_t max_size)
{
    struct queue* queue = malloc(sizeof(*queue));
    /* omp_init_lock(&queue->lock); */
    c_list_init(&queue->tasks);
    queue->ntasks = 0;
    queue->max_size = max_size;
    /* omp_init_lock(&queue->full); */
    /* omp_init_lock(&queue->empty); */
    /* omp_set_lock(&queue->empty); */
    return queue;
}

/* void queue_wait_not_full(struct queue* queue) { omp_set_lock(&queue->full); } */

/* void queue_wait_not_empty(struct queue* queue) { omp_set_lock(&queue->empty); } */

/* void queue_lock_push(struct queue* queue) { omp_set_lock(&queue->push_lock); } */

/* void queue_unlock_push(struct queue* queue) { omp_unset_lock(&queue->push_lock); } */

void queue_push(struct queue* queue, struct task* task)
{
    /* omp_set_lock(&queue->lock); */

    c_list_link_tail(&queue->tasks, &task->link);
    queue->ntasks++;

    /* omp_unset_lock(&queue->lock); */
}

struct task* queue_pop(struct queue* queue)
{
    /* omp_set_lock(&queue->lock); */

    struct CList* elem = c_list_first(&queue->tasks);
    struct task*  task = NULL;
    if (elem) {
        task = c_list_entry(elem, struct task, link);
        c_list_unlink(elem);
        queue->ntasks--;
    }

    /* omp_unset_lock(&queue->lock); */
    return task;
}

uint32_t queue_size(struct queue* queue) { return queue->ntasks; }

int dcp_master(char const* db_filepath, char const* seq_str)
{
    struct dcp_input*     input = dcp_input_create(db_filepath);
    struct dcp_partition* part = dcp_input_create_partition(input, 0, 1);
    struct queue*         queue = queue_create(2);
    bool                  finished = false;
    omp_lock_t            push_lock;
    omp_lock_t            pop_lock;
    omp_init_lock(&push_lock);
    omp_init_lock(&pop_lock);
    omp_set_lock(&pop_lock);

#pragma omp parallel default(none) shared(part, seq_str, queue, finished, push_lock, pop_lock) /* if(1 == 2) */
    {
#pragma omp master
        {
            int i = 0;
            while (!dcp_partition_end(part)) {
                /* printf("Read profile %d\n", i); */
                printf("Producer\n");
                i++;
                struct nmm_profile const* prof = dcp_partition_read(part);
                struct task*              task = task_create(prof);

                omp_set_lock(&push_lock);

                printf("Producer 1\n");
#pragma omp        critical(queue)
                {
                    printf("Producer 2\n");
                    omp_unset_lock(&push_lock);
                    printf("Producer 3\n");
                    queue_push(queue, task);
                    printf("Producer 4\n");
                    if (queue_size(queue) == 4)
                        omp_set_lock(&push_lock);
                    printf("Producer 5\n");
                    if (queue_size(queue) == 1) {
#pragma omp critical(pop)
                        {
                            omp_unset_lock(&pop_lock);
                        }
                    }
                    printf("Producer 6\n");
                }
            }
#pragma omp atomic write
            finished = true;
            printf("FINISHED\n");
        }

        while (true) {
            struct task* task = NULL;
            printf("Consumer\n");

#pragma omp critical(pop)
            {
                omp_set_lock(&pop_lock);
            }

            printf("Consumer 1\n");
#pragma omp critical(queue)
            {
                printf("Consumer 2\n");

#pragma omp critical(pop)
                {
                    omp_unset_lock(&pop_lock);
                }
                printf("Consumer 3\n");
                task = queue_pop(queue);
                printf("Consumer 4\n");
                if (queue_size(queue) == 3)
                    omp_unset_lock(&push_lock);
                printf("Consumer 5\n");
                if (queue_size(queue) == 0) {
#pragma omp critical(pop)
                    {
                        omp_set_lock(&pop_lock);
                    }
                }
                printf("Consumer 6\n");
            }
            if (task) {
                printf("process\n");
                process(seq_str, task->profile);
            } else
                printf(".");
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
