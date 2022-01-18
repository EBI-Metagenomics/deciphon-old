#include "rest.h"
#include "common/compiler.h"
#include "common/jsmn.h"
#include "common/logger.h"
#include "common/rc.h"
#include "common/safe.h"
#include "common/to.h"
#include "common/xmath.h"
#include <curl/curl.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static char rest_url[1024] = {0};
static char base_url[1024] = {0};
// "ret": {
//   "rc": "done",
//   "error": ""
// }
//
static CURL *curl = 0;

static struct answer
{
    struct
    {
        jsmn_parser parser;
        jsmntok_t tok[128];
        unsigned ntoks;
    } json;
    char *data;
    size_t size;
} answer = {0};

struct rest_ret rest_ret = {0};

struct rest_job_state job_state = {0};

static size_t answer_callback(void *contents, size_t size, size_t nmemb,
                              void *userp)
{
    struct answer *a = (struct answer *)&answer;
    a->data = contents;
    a->size = size * nmemb;

    // char *ptr = realloc(a->data, a->size + realsize + 1);
    // if (!ptr)
    // {
    //     error(RC_ENOMEM, "failed to realloc");
    //     return 0;
    // }

    // a->data = ptr;
    // memcpy(&(a->data[a->size]), contents, realsize);
    // a->size += realsize;
    // a->data[a->size] = 0;

    jsmn_init(&a->json.parser);
    int r = jsmn_parse(&a->json.parser, a->data, a->size, a->json.tok, 128);
    if (r < 0)
    {
        eparse("parse json");
        return a->size;
    }
    /* Assume the top-level element is an object */
    if (r < 1 || a->json.tok[0].type != JSMN_OBJECT)
    {
        error(RC_EPARSE, "object expected");
        return 0;
    }

    a->json.ntoks = (unsigned)r;
    return a->size;
}

enum rc rest_open(char const *url)
{
    safe_strcpy(base_url, url, ARRAY_SIZE(base_url));

    struct answer *a = (struct answer *)&answer;

    a->data = malloc(1);
    a->size = 0;
    curl = curl_easy_init();
    if (!curl) return efail("curl_easy_init");

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Accept: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, answer_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, 0);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");

    return RC_DONE;
}

void rest_close(void)
{
    curl_easy_cleanup(curl);
    free(answer.data);
    curl_global_cleanup();
}

static inline unsigned tokl(jsmntok_t const *tok)
{
    return (unsigned)(tok->end - tok->start);
}

static inline char const *tokv(jsmntok_t const *tok)
{
    return answer.data + tok->start;
}

static void cpy_str(unsigned dst_sz, char *dst, jsmntok_t const *tok)
{
    unsigned len = xmath_min(dst_sz - 1, tokl(tok));
    memcpy(dst, tokv(tok), len);
    dst[len] = 0;
}

static bool jeq(jsmntok_t const *tok, const char *s)
{
    char const *str = tokv(tok);
    unsigned len = tokl(tok);
    return (tok->type == JSMN_STRING && (unsigned)strlen(s) == len &&
            strncmp(str, s, len) == 0);
}

static enum rc parse_rc(jsmntok_t const *tok, enum rc *rc)
{
    return rc_from_str(tokl(tok), tokv(tok), rc);
}

static enum rc parse_ret(unsigned nitems, jsmntok_t const *tok)
{
    if (nitems != 2) return error(RC_EINVAL, "expected two items");

    enum rc rc = RC_DONE;
    for (unsigned i = 1; i < nitems * 2; i += 2)
    {
        if (jeq(tok + i, "rc"))
        {
            if ((rc = parse_rc(tok + i + 1, &rest_ret.rc))) return rc;
        }
        else if (jeq(tok + i, "error"))
        {
            cpy_str(JOB_ERROR_SIZE, rest_ret.error, tok + i + 1);
        }
        else
            return error(RC_EINVAL, "unexpected json key");
    }
    return RC_DONE;
}

static enum rc parse_filepath(unsigned ntoks, jsmntok_t const *tok,
                              unsigned path_size, char *path)
{
    enum rc rc = RC_DONE;
    for (unsigned i = 1; i < ntoks; i += 2)
    {
        if (jeq(tok + i, "ret"))
        {
            if (tok[i + 1].type != JSMN_OBJECT)
                return error(RC_EPARSE, "expected json object");

            unsigned nitems = (unsigned)tok[i + 1].size;
            if ((rc = parse_ret(nitems, tok + i + 1))) return rc;
            i += (unsigned)(nitems * 2);
        }
        else if (jeq(tok + i, "filepath"))
        {
            cpy_str(path_size, path, tok + i + 1);
        }
        else
            return error(RC_EINVAL, "unexpected json key");
    }
    return RC_DONE;
}

enum rc rest_set_job_fail(int64_t job_id, char const *error)
{
    sprintf(rest_url, "%s/job/set_fail?job_id=%" PRId64, base_url, job_id);
    curl_easy_setopt(curl, CURLOPT_URL, rest_url);

    if (curl_easy_perform(curl) != CURLE_OK) return efail("curl_easy_perform");

    printf("%.*s\n", (int)answer.size, answer.data);
    // return parse_filepath(answer.json.ntoks, answer.json.tok, path_size, path);
}

enum rc rest_set_job_done(int64_t job_id) {}

enum rc rest_get_db_filepath(unsigned path_size, char *path, int64_t id)
{
    sprintf(rest_url, "%s/db/filepath?db_id=%" PRId64, base_url, id);
    curl_easy_setopt(curl, CURLOPT_URL, rest_url);

    if (curl_easy_perform(curl) != CURLE_OK) return efail("curl_easy_perform");

    printf("%.*s\n", (int)answer.size, answer.data);
    return parse_filepath(answer.json.ntoks, answer.json.tok, path_size, path);
}

// static enum rc parse_job_state(void)
// {
//     struct answer *a = (struct answer *)&answer;
//     enum rc rc = RC_DONE;
//     for (unsigned i = 1; i < (unsigned)a->json.ntoks; i++)
//     {
//         if (jeq(&a->json.tok[i], "rc") == 0)
//         {
//             if ((rc = parse_rc(a->json.tok + i + 1, &rest_rc))) return rc;
//             i++;
//         }
//         else if (jeq(&a->json.tok[i], "error") == 0)
//         {
//             parse_error(a->json.tok + i + 1, rest_error);
//             i++;
//         }
//         else if (jeq(&a->json.tok[i], "state") == 0)
//         {
//             unsigned len =
//                 xmath_min(JOB_STATE_SIZE - 1, tokl(a->json.tok + i + 1));
//             memcpy(job_state.state, tokv(a->json.tok + i + 1), len);
//             job_state.state[len] = 0;
//             i++;
//         }
//         else
//         {
//             printf("Unexpected key: %.*s\n", tokl(a->json.tok + i),
//                    tokv(a->json.tok + i));
//             return error(RC_EINVAL, "unexpected json key");
//         }
//     }
//     return RC_DONE;
// }

// enum rc rest_job_state(int64_t job_id)
// {
//     struct answer *a = (struct answer *)&answer;
//     jsmn_init(&a->json.parser);
//     a->data = malloc(1);
//     a->size = 0;
//     CURL *curl = curl_easy_init();
//     if (!curl) return efail("curl_easy_init");
//     struct curl_slist *headers = NULL;
//     headers = curl_slist_append(headers, "Accept: application/json");
//
//     sprintf(rest_url, "%s/job/status?job_id=%" PRId64, base_url, job_id);
//     curl_easy_setopt(curl, CURLOPT_URL, rest_url);
//
//     curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
//     curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, answer_callback);
//     curl_easy_setopt(curl, CURLOPT_WRITEDATA, 0);
//     curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
//     CURLcode res = curl_easy_perform(curl);
//
//     if (res != CURLE_OK)
//     {
//         fprintf(stderr, "curl_easy_perform() failed: %s\n",
//                 curl_easy_strerror(res));
//     }
//     else
//     {
//         printf("%lu bytes retrieved\n", (unsigned long)a->size);
//         printf("%s\n", a->data);
//         parse_job_state();
//     }
//
//     curl_easy_cleanup(curl);
//     free(a->data);
//
//     curl_global_cleanup();
//     return RC_DONE;
// }

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

static enum rc parse_pend_job(unsigned nitems, jsmntok_t const *tok,
                              struct rest_pend_job *job)
{
    if (nitems != 4) return error(RC_EINVAL, "expected four items");

    for (unsigned i = 1; i < nitems * 2; i += 2)
    {
        if (jeq(tok + i, "id"))
        {
            if (!is_number(tok + i + 1))
                return error(RC_EINVAL, "expected number");
            if (!to_int64l(tokl(tok + i + 1), tokv(tok + i + 1), &job->id))
                return eparse("parse number");
        }
        else if (jeq(tok + i, "db_id"))
        {
            if (!is_number(tok + i + 1))
                return error(RC_EINVAL, "expected number");
            if (!to_int64l(tokl(tok + i + 1), tokv(tok + i + 1), &job->db_id))
                return eparse("parse number");
        }
        else if (jeq(tok + i, "multi_hits"))
        {
            if (!is_boolean(tok + i + 1))
                return error(RC_EINVAL, "expected bool");
            job->multi_hits = to_bool(tok + i + 1);
        }
        else if (jeq(tok + i, "hmmer3_compat"))
        {
            if (!is_boolean(tok + i + 1))
                return error(RC_EINVAL, "expected bool");
            job->hmmer3_compat = to_bool(tok + i + 1);
        }
        else
            return error(RC_EINVAL, "unexpected json key");
    }
    return RC_DONE;
}

static enum rc parse_next_pend_job(unsigned ntoks, jsmntok_t const *tok,
                                   struct rest_pend_job *job)
{
    enum rc rc = RC_DONE;
    for (unsigned i = 1; i < ntoks; i += 2)
    {
        if (jeq(tok + i, "ret"))
        {
            if (tok[i + 1].type != JSMN_OBJECT)
                return error(RC_EPARSE, "expected json object");

            unsigned nitems = (unsigned)tok[i + 1].size;
            if ((rc = parse_ret(nitems, tok + i + 1))) return rc;
            i += (unsigned)(nitems * 2);
        }
        else if (jeq(tok + i, "job"))
        {
            if (tok[i + 1].type != JSMN_OBJECT)
                return error(RC_EPARSE, "expected json object");

            unsigned nitems = (unsigned)tok[i + 1].size;
            if ((rc = parse_pend_job(nitems, tok + i + 1, job))) return rc;
            i += (unsigned)(nitems * 2);
        }
        else
            return error(RC_EINVAL, "unexpected json key");
    }
    return RC_DONE;
}

enum rc rest_next_pend_job(struct rest_pend_job *job)
{
    sprintf(rest_url, "%s/job/next_pend", base_url);
    curl_easy_setopt(curl, CURLOPT_URL, rest_url);

    if (curl_easy_perform(curl) != CURLE_OK) return efail("curl_easy_perform");

    printf("%.*s\n", (int)answer.size, answer.data);
    return parse_next_pend_job(answer.json.ntoks, answer.json.tok, job);
}

static enum rc parse_seq(unsigned nitems, jsmntok_t const *tok,
                         struct sched_seq *job)
{
    if (nitems != 4) return error(RC_EINVAL, "expected four items");

    for (unsigned i = 1; i < nitems * 2; i += 2)
    {
        if (jeq(tok + i, "id"))
        {
            if (!is_number(tok + i + 1))
                return error(RC_EINVAL, "expected number");
            if (!to_int64l(tokl(tok + i + 1), tokv(tok + i + 1), &job->id))
                return eparse("parse number");
        }
        else if (jeq(tok + i, "job_id"))
        {
            if (!is_number(tok + i + 1))
                return error(RC_EINVAL, "expected number");
            if (!to_int64l(tokl(tok + i + 1), tokv(tok + i + 1), &job->job_id))
                return eparse("parse number");
        }
        else if (jeq(tok + i, "name"))
        {
            cpy_str(SEQ_NAME_SIZE, job->name, tok + i + 1);
        }
        else if (jeq(tok + i, "data"))
        {
            cpy_str(SEQ_SIZE, job->data, tok + i + 1);
        }
        else
            return error(RC_EINVAL, "unexpected json key");
    }
    return RC_DONE;
}

static enum rc parse_next_seq(unsigned ntoks, jsmntok_t const *tok,
                              struct sched_seq *seq)
{
    enum rc rc = RC_DONE;
    for (unsigned i = 1; i < ntoks; i += 2)
    {
        if (jeq(tok + i, "ret"))
        {
            if (tok[i + 1].type != JSMN_OBJECT)
                return error(RC_EPARSE, "expected json object");

            unsigned nitems = (unsigned)tok[i + 1].size;
            if ((rc = parse_ret(nitems, tok + i + 1))) return rc;
            i += (unsigned)(nitems * 2);
        }
        else if (jeq(tok + i, "seq"))
        {
            if (tok[i + 1].type != JSMN_OBJECT)
                return error(RC_EPARSE, "expected json object");

            unsigned nitems = (unsigned)tok[i + 1].size;
            if ((rc = parse_seq(nitems, tok + i + 1, seq))) return rc;
            i += (unsigned)(nitems * 2);
        }
        else
            return error(RC_EINVAL, "unexpected json key");
    }
    return RC_DONE;
}

// HEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEREEEEEEEEEEEEEEEEEEEEEEEEEEEE
// long http_code = 0;
// curl_easy_getinfo (session, CURLINFO_RESPONSE_CODE, &http_code);

enum rc rest_next_seq(struct sched_seq *seq)
{
    sprintf(rest_url, "%s/seq/next?seq_id=%" PRId64 "&job_id=%" PRId64,
            base_url, seq->id, seq->job_id);
    curl_easy_setopt(curl, CURLOPT_URL, rest_url);

    if (curl_easy_perform(curl) != CURLE_OK) return efail("curl_easy_perform");

    printf("%.*s\n", (int)answer.size, answer.data);
    return parse_next_seq(answer.json.ntoks, answer.json.tok, seq);
}
