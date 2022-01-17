#include "rest.h"
#include "common/compiler.h"
#include "common/jsmn.h"
#include "common/logger.h"
#include "common/rc.h"
#include "common/to.h"
#include "common/xmath.h"
#include <curl/curl.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static struct answer
{
    struct
    {
        jsmn_parser parser;
        jsmntok_t tok[128];
        int r;
    } json;
    char *data;
    size_t size;
} answer = {0};

struct rest_job_state job_state = {0};

enum rc rest_set_job_fail(int64_t job_id, char const *error) {}

enum rc rest_set_job_done(int64_t job_id) {}

enum rc rest_get_db_filepath(unsigned size, char *filepath, int64_t id) {}

enum rc rest_seq_next(struct sched_seq *seq) {}

static size_t answer_callback(void *contents, size_t size, size_t nmemb,
                              void *userp)
{
    size_t realsize = size * nmemb;
    struct answer *a = (struct answer *)&answer;

    char *ptr = realloc(a->data, a->size + realsize + 1);
    if (!ptr)
    {
        /* out of memory! */
        printf("not enough memory (realloc returned NULL)\n");
        return 0;
    }

    a->data = ptr;
    memcpy(&(a->data[a->size]), contents, realsize);
    a->size += realsize;
    a->data[a->size] = 0;

    a->json.r =
        jsmn_parse(&a->json.parser, a->data, strlen(a->data), a->json.tok, 128);
    if (a->json.r < 0)
    {
        printf("Failed to parse JSON: %d\n", a->json.r);
        return realsize;
    }
    /* Assume the top-level element is an object */
    if (a->json.r < 1 || a->json.tok[0].type != JSMN_OBJECT)
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

static inline unsigned tokl(jsmntok_t const *tok)
{
    return (unsigned)(tok->end - tok->start);
}

static inline char const *tokv(jsmntok_t const *tok)
{
    return answer.data + tok->start;
}

static enum rc parse_rc(jsmntok_t const *tok, enum rc *rc)
{
    return rc_from_str(tokl(tok), tokv(tok), rc);
}

static void parse_error(jsmntok_t const *tok, char *error)
{
    unsigned len = xmath_min(JOB_ERROR_SIZE - 1, tokl(tok));
    memcpy(error, tokv(tok), len);
    error[len] = 0;
}

static enum rc parse_job_state(void)
{
    struct answer *a = (struct answer *)&answer;
    enum rc rc = RC_DONE;
    for (unsigned i = 1; i < (unsigned)a->json.r; i++)
    {
        if (jsoneq(a->data, &a->json.tok[i], "rc") == 0)
        {
            if ((rc = parse_rc(a->json.tok + i + 1, &job_state.rc))) return rc;
            i++;
        }
        else if (jsoneq(a->data, &a->json.tok[i], "error") == 0)
        {
            parse_error(a->json.tok + i + 1, job_state.error);
            i++;
        }
        else if (jsoneq(a->data, &a->json.tok[i], "state") == 0)
        {
            unsigned len =
                xmath_min(JOB_STATE_SIZE - 1, tokl(a->json.tok + i + 1));
            memcpy(job_state.state, tokv(a->json.tok + i + 1), len);
            job_state.state[len] = 0;
            i++;
        }
        else
        {
            printf("Unexpected key: %.*s\n", tokl(a->json.tok + i),
                   tokv(a->json.tok + i));
            return error(RC_EINVAL, "unexpected json key");
        }
    }
    return RC_DONE;
}

char url[1024] = {0};

enum rc rest_job_state(int64_t job_id)
{
    struct answer *a = (struct answer *)&answer;
    jsmn_init(&a->json.parser);
    a->data = malloc(1);
    a->size = 0;
    CURL *curl = curl_easy_init();
    if (!curl) return efail("curl_easy_init");
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Accept: application/json");

    sprintf(url, "http://127.0.0.1:8000/job/status?job_id=%" PRId64, job_id);
    curl_easy_setopt(curl, CURLOPT_URL, url);

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, answer_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, 0);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
    CURLcode res = curl_easy_perform(curl);

    if (res != CURLE_OK)
    {
        fprintf(stderr, "curl_easy_perform() failed: %s\n",
                curl_easy_strerror(res));
    }
    else
    {
        printf("%lu bytes retrieved\n", (unsigned long)a->size);
        printf("%s\n", a->data);
        parse_job_state();
    }

    curl_easy_cleanup(curl);
    free(a->data);

    curl_global_cleanup();
    return RC_DONE;
}

static bool is_number(jsmntok_t const *tok)
{
    char *c = answer.data + tok->start;
    return (tok->type == JSMN_PRIMITIVE && *c != 'n' && *c != 't' && *c != 'f');
}

static bool is_boolean(jsmntok_t const *tok)
{
    char *c = answer.data + tok->start;
    return (tok->type == JSMN_PRIMITIVE && (*c == 't' || *c == 'f'));
}

static bool to_bool(jsmntok_t const *tok)
{
    char *c = answer.data + tok->start;
    return *c == 't';
}

static enum rc parse_pend_job(jsmntok_t *tok, unsigned size,
                              struct rest_pend_job *job)
{
    if (size != 4) return error(RC_EINVAL, "expected four keys");

    struct answer *a = (struct answer *)&answer;
    for (unsigned i = 1; i < size * 2; ++i)
    {
        if (jsoneq(a->data, &tok[i], "id") == 0)
        {
            if (!is_number(tok + i + 1))
                return error(RC_EINVAL, "expected number");
            if (!to_int64l(tokl(tok + i + 1), tokv(tok + i + 1), &job->id))
                return eparse("parse number");
            i++;
        }
        else if (jsoneq(a->data, &tok[i], "db_id") == 0)
        {
            if (!is_number(tok + i + 1))
                return error(RC_EINVAL, "expected number");
            if (!to_int64l(tokl(tok + i + 1), tokv(tok + i + 1), &job->db_id))
                return eparse("parse number");
            i++;
        }
        else if (jsoneq(a->data, &tok[i], "multi_hits") == 0)
        {
            if (!is_boolean(tok + i + 1))
                return error(RC_EINVAL, "expected bool");
            job->multi_hits = to_bool(tok + i + 1);
            i++;
        }
        else if (jsoneq(a->data, &tok[i], "hmmer3_compat") == 0)
        {
            if (!is_boolean(tok + i + 1))
                return error(RC_EINVAL, "expected bool");
            job->hmmer3_compat = to_bool(tok + i + 1);
            i++;
        }
        else
        {
            printf("Unexpected key: %.*s\n", tokl(a->json.tok + i),
                   tokv(a->json.tok + i));
            return error(RC_EINVAL, "unexpected json key");
        }
    }
    return RC_DONE;
}

static enum rc parse_next_pend_job(struct rest_pend_job *job)
{
    struct answer *a = (struct answer *)&answer;
    enum rc rc = RC_DONE;
    for (unsigned i = 1; i < (unsigned)a->json.r; i++)
    {
        if (jsoneq(a->data, &a->json.tok[i], "rc") == 0)
        {
            if ((rc = parse_rc(a->json.tok + i + 1, &job_state.rc))) return rc;
            i++;
        }
        else if (jsoneq(a->data, &a->json.tok[i], "error") == 0)
        {
            parse_error(a->json.tok + i + 1, job_state.error);
            i++;
        }
        else if (jsoneq(a->data, &a->json.tok[i], "job") == 0)
        {
            if (a->json.tok[i + 1].type != JSMN_OBJECT)
                return error(RC_EPARSE, "expected json object");

            int size = a->json.tok[i + 1].size;
            rc = parse_pend_job(a->json.tok + i + 1, (unsigned)size, job);
            if (rc) return rc;
            i += (unsigned)(size * 2);
            ++i;
        }
        else
        {
            printf("Unexpected key: %.*s\n", tokl(a->json.tok + i),
                   tokv(a->json.tok + i));
            return error(RC_EINVAL, "unexpected json key");
        }
    }
    return RC_DONE;
}

enum rc rest_next_pend_job(struct rest_pend_job *job)
{
    struct answer *a = (struct answer *)&answer;
    jsmn_init(&a->json.parser);
    a->data = malloc(1);
    a->size = 0;
    CURL *curl = curl_easy_init();
    if (!curl) return efail("curl_easy_init");
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Accept: application/json");

    sprintf(url, "http://127.0.0.1:8000/job/next_pend");
    curl_easy_setopt(curl, CURLOPT_URL, url);

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, answer_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, 0);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
    CURLcode res = curl_easy_perform(curl);

    if (res != CURLE_OK)
    {
        fprintf(stderr, "curl_easy_perform() failed: %s\n",
                curl_easy_strerror(res));
    }
    else
    {
        printf("%lu bytes retrieved\n", (unsigned long)a->size);
        printf("%s\n", a->data);
        parse_next_pend_job(job);
    }

    curl_easy_cleanup(curl);
    free(a->data);

    curl_global_cleanup();
    return RC_DONE;
}

static enum rc parse_seq(jsmntok_t *tok, unsigned size, struct sched_seq *job)
{
    if (size != 4) return error(RC_EINVAL, "expected four keys");

    struct answer *a = (struct answer *)&answer;
    for (unsigned i = 1; i < size * 2; ++i)
    {
        if (jsoneq(a->data, &tok[i], "id") == 0)
        {
            if (!is_number(tok + i + 1))
                return error(RC_EINVAL, "expected number");
            if (!to_int64l(tokl(tok + i + 1), tokv(tok + i + 1), &job->id))
                return eparse("parse number");
            i++;
        }
        else if (jsoneq(a->data, &tok[i], "job_id") == 0)
        {
            if (!is_number(tok + i + 1))
                return error(RC_EINVAL, "expected number");
            if (!to_int64l(tokl(tok + i + 1), tokv(tok + i + 1), &job->job_id))
                return eparse("parse number");
            i++;
        }
        else if (jsoneq(a->data, &tok[i], "name") == 0)
        {
            unsigned len = xmath_min(SEQ_NAME_SIZE - 1, tokl(tok));
            memcpy(job->name, tokv(tok), len);
            job->name[len] = 0;
            i++;
        }
        else if (jsoneq(a->data, &tok[i], "data") == 0)
        {
            unsigned len = xmath_min(SEQ_SIZE - 1, tokl(tok));
            memcpy(job->data, tokv(tok), len);
            job->data[len] = 0;
            i++;
        }
        else
        {
            printf("Unexpected key: %.*s\n", tokl(a->json.tok + i),
                   tokv(a->json.tok + i));
            return error(RC_EINVAL, "unexpected json key");
        }
    }
    return RC_DONE;
}

static enum rc parse_next_seq(struct sched_seq *seq)
{
    struct answer *a = (struct answer *)&answer;
    enum rc rc = RC_DONE;
    for (unsigned i = 1; i < (unsigned)a->json.r; i++)
    {
        if (jsoneq(a->data, &a->json.tok[i], "rc") == 0)
        {
            if ((rc = parse_rc(a->json.tok + i + 1, &job_state.rc))) return rc;
            i++;
        }
        else if (jsoneq(a->data, &a->json.tok[i], "error") == 0)
        {
            parse_error(a->json.tok + i + 1, job_state.error);
            i++;
        }
        else if (jsoneq(a->data, &a->json.tok[i], "seq") == 0)
        {
            if (a->json.tok[i + 1].type != JSMN_OBJECT)
                return error(RC_EPARSE, "expected json object");

            int size = a->json.tok[i + 1].size;
            rc = parse_seq(a->json.tok + i + 1, (unsigned)size, seq);
            if (rc) return rc;
            i += (unsigned)(size * 2);
            ++i;
        }
        else
        {
            printf("Unexpected key: %.*s\n", tokl(a->json.tok + i),
                   tokv(a->json.tok + i));
            return error(RC_EINVAL, "unexpected json key");
        }
    }
    return RC_DONE;
}

enum rc rest_next_seq(struct sched_seq *seq)
{
    struct answer *a = (struct answer *)&answer;
    jsmn_init(&a->json.parser);
    a->data = malloc(1);
    a->size = 0;
    CURL *curl = curl_easy_init();
    if (!curl) return efail("curl_easy_init");
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Accept: application/json");

    sprintf(url,
            "http://127.0.0.1:8000/seq/next?seq_id=%" PRId64 "&job_id=%" PRId64,
            seq->id, seq->job_id);
    curl_easy_setopt(curl, CURLOPT_URL, url);

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, answer_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, 0);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
    CURLcode res = curl_easy_perform(curl);

    if (res != CURLE_OK)
    {
        fprintf(stderr, "curl_easy_perform() failed: %s\n",
                curl_easy_strerror(res));
    }
    else
    {
        printf("%lu bytes retrieved\n", (unsigned long)a->size);
        printf("%s\n", a->data);
        parse_next_seq(seq);
    }

    curl_easy_cleanup(curl);
    free(a->data);

    curl_global_cleanup();
    return RC_DONE;
}
