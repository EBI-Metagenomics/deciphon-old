#include "server.h"
#include "cco/cco.h"
#include "common/compiler.h"
#include "common/jsmn.h"
#include "common/logger.h"
#include "common/rc.h"
#include "common/safe.h"
#include "common/xfile.h"
#include "common/xmath.h"
#include "db.h"
#include "elapsed/elapsed.h"
#include "job.h"
#include "protein_state.h"
#include "sched.h"
#include "sched/sched.h"
#include "table.h"
#include "work.h"
#include <curl/curl.h>
#include <signal.h>

static struct server
{
    struct
    {
        volatile sig_atomic_t interrupt;
        struct sigaction action;
    } signal;
    unsigned num_threads;
    struct job job;
    struct work work;
} server = {0};

static void signal_interrupt(int signum) { server.signal.interrupt = 1; }

enum rc server_open(char const *filepath, unsigned num_threads)
{
    server.signal.action.sa_handler = &signal_interrupt;
    sigemptyset(&server.signal.action.sa_mask);
    sigaction(SIGINT, &server.signal.action, NULL);
    server.num_threads = num_threads;

    if (sched_setup(filepath)) return error(RC_EFAIL, "failed to setup sched");
    if (sched_open()) return error(RC_EFAIL, "failed to open sched");
    server.work.lrt_threshold = 100.0f;
    return RC_DONE;
}

enum rc server_close(void) { return sched_close() ? RC_EFAIL : RC_DONE; }

enum rc server_add_db(char const *filepath, int64_t *id)
{
    if (!xfile_is_readable(filepath))
        return error(RC_EIO, "file is not readable");

    if (sched_add_db(filepath, id)) return error(RC_EFAIL, "failed to add db");
    return RC_DONE;
}

enum rc server_submit_job(struct job *job)
{
    struct sched_job j = {0};
    sched_job_init(&j, job->db_id, job->multi_hits, job->hmmer3_compat);
    if (sched_begin_job_submission(&j))
        return error(RC_EFAIL, "failed to begin job submission");

    struct seq *seq = NULL;
    struct cco_iter iter = cco_queue_iter(&job->seqs);
    cco_iter_for_each_entry(seq, &iter, node)
    {
        sched_add_seq(&j, seq->name, seq->str.data);
    }

    if (sched_end_job_submission(&j))
        return error(RC_EFAIL, "failed to end job submission");

    job->id = j.id;
    return RC_DONE;
}

enum rc server_job_state(int64_t job_id, enum sched_job_state *state)
{
    if (sched_job_state(job_id, state))
        return error(RC_EFAIL, "failed to get job state");
    return RC_DONE;
}

struct Answer
{
    jsmn_parser p;
    jsmntok_t t[128];
    char *data;
    size_t size;
    int r;
};

static size_t answer_callback(void *contents, size_t size, size_t nmemb,
                              void *userp)
{
    size_t realsize = size * nmemb;
    struct Answer *mem = (struct Answer *)userp;

    char *ptr = realloc(mem->data, mem->size + realsize + 1);
    if (!ptr)
    {
        /* out of memory! */
        printf("not enough memory (realloc returned NULL)\n");
        return 0;
    }

    mem->data = ptr;
    memcpy(&(mem->data[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->data[mem->size] = 0;

    mem->r = jsmn_parse(&mem->p, mem->data, strlen(mem->data), mem->t, 128);
    if (mem->r < 0)
    {
        printf("Failed to parse JSON: %d\n", mem->r);
        return realsize;
    }
    /* Assume the top-level element is an object */
    if (mem->r < 1 || mem->t[0].type != JSMN_OBJECT)
    {
        printf("Object expected\n");
        return realsize;
    }

    return realsize;
}
static int jsoneq(const char *json, jsmntok_t *tok, const char *s)
{
    if (tok->type == JSMN_STRING && (int)strlen(s) == tok->end - tok->start &&
        strncmp(json + tok->start, s, (unsigned)(tok->end - tok->start)) == 0)
    {
        return 0;
    }
    return -1;
}
struct Answer chunk = {0};

struct job_status
{
    enum rc rc;
    char error[JOB_ERROR_SIZE];
    char state[JOB_STATE_SIZE];
} job_status = {0};

static void parse_job_status(void)
{
    for (int i = 1; i < chunk.r; i++)
    {
        if (jsoneq(chunk.data, &chunk.t[i], "rc") == 0)
        {
            int len = chunk.t[i + 1].end - chunk.t[i + 1].start;
            rc_from_str((unsigned)len, chunk.data + chunk.t[i + 1].start,
                        &job_status.rc);
            i++;
        }
        else if (jsoneq(chunk.data, &chunk.t[i], "error") == 0)
        {
            /* We may additionally check if the value is either "true" or
             * "false" */
            unsigned len =
                xmath_min(JOB_ERROR_SIZE - 1, (unsigned)(chunk.t[i + 1].end -
                                                         chunk.t[i + 1].start));
            memcpy(job_status.error, chunk.data + chunk.t[i + 1].start, len);
            job_status.error[len] = 0;
            i++;
        }
        else if (jsoneq(chunk.data, &chunk.t[i], "state") == 0)
        {
            unsigned len =
                xmath_min(JOB_STATE_SIZE - 1, (unsigned)(chunk.t[i + 1].end -
                                                         chunk.t[i + 1].start));
            memcpy(job_status.state, chunk.data + chunk.t[i + 1].start, len);
            job_status.state[len] = 0;
            i++;
        }
        else
        {
            printf("Unexpected key: %.*s\n", chunk.t[i].end - chunk.t[i].start,
                   chunk.data + chunk.t[i].start);
        }
    }
}

enum rc server_run(bool single_run)
{
    jsmn_init(&chunk.p);
    chunk.data = malloc(1); /* will be grown as needed by the realloc above */
    chunk.size = 0;         /* no data at this point */
    CURL *curl = curl_easy_init();
    if (!curl) return 0;
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Accept: application/json");
    curl_easy_setopt(curl, CURLOPT_URL,
                     "http://127.0.0.1:8000/job/status?job_id=1");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, answer_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
    CURLcode res = curl_easy_perform(curl);

    /* check for errors */
    if (res != CURLE_OK)
    {
        fprintf(stderr, "curl_easy_perform() failed: %s\n",
                curl_easy_strerror(res));
    }
    else
    {
        /*
         * Now, our chunk.memory points to a memory block that is chunk.size
         * bytes big and contains the remote file.
         *
         * Do something nice with it!
         */

        printf("%lu bytes retrieved\n", (unsigned long)chunk.size);
        printf("%s\n", chunk.data);
        parse_job_status();
        printf("%d\n", job_status.rc);
        printf("%s\n", job_status.error);
        printf("%s\n", job_status.state);
    }

    curl_easy_cleanup(curl);
    free(chunk.data);

    /* we are done with libcurl, so clean it up */
    curl_global_cleanup();
    return 0;

#if 0
    enum rc rc = RC_DONE;

    info("Starting the server");
    while (!server.signal.interrupt)
    {
        if ((rc = work_next(&server.work)) == RC_NOTFOUND)
        {
            if (single_run) break;
            elapsed_sleep(500);
            continue;
        }
        if (rc != RC_NEXT) return rc;

        info("Found a new job");
        rc = work_run(&server.work, server.num_threads);
        if (rc) return rc;
        info("Finished a job");
    }

    info("Goodbye!");
    return RC_DONE;
#endif
}

void server_set_lrt_threshold(imm_float lrt)
{
    server.work.lrt_threshold = lrt;
}

enum rc server_get_sched_job(struct sched_job *job)
{
    if (sched_get_job(job)) return error(RC_EFAIL, "failed to get job");
    return RC_DONE;
}

enum rc server_next_sched_prod(struct sched_job const *job,
                               struct sched_prod *prod)
{
    prod->job_id = job->id;
    enum rc rc = sched_prod_next(prod);
    if (rc == RC_DONE) return RC_DONE;
    if (rc != RC_NEXT) return error(RC_EFAIL, "failed to get prod");
    return RC_NEXT;
}
