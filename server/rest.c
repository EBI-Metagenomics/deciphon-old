#include "deciphon/server/rest.h"
#include "deciphon/server/rest_curl.h"
#include "deciphon/server/sched.h"
#include "deciphon/spinlock.h"
#include "jsmn.h"
#include <inttypes.h>

static struct rest_curl curl = {0};
static spinlock_t lock = SPINLOCK_INIT;

// static struct answer
// {
//     struct
//     {
//         jsmn_parser parser;
//         jsmntok_t tok[128];
//         unsigned ntoks;
//     } json;
//     char *data;
//     size_t size;
// } answer = {0};

struct
{
    jsmn_parser parser;
    jsmntok_t tok[128];
    unsigned ntoks;
} json;

static inline unsigned tokl(jsmntok_t const *tok)
{
    return (unsigned)(tok->end - tok->start);
}

static inline char const *tokv(char *data, jsmntok_t const *tok)
{
    return data + tok->start;
}

static bool jeq(char *data, jsmntok_t const *tok, const char *s)
{
    char const *str = tokv(data, tok);
    unsigned len = tokl(tok);
    return (tok->type == JSMN_STRING && (unsigned)strlen(s) == len &&
            strncmp(str, s, len) == 0);
}

static bool is_number(char *data, jsmntok_t const *tok)
{
    char *c = data + tok->start;
    return (tok->type == JSMN_PRIMITIVE && *c != 'n' && *c != 't' && *c != 'f');
}

static bool is_boolean(char *data, jsmntok_t const *tok)
{
    char *c = data + tok->start;
    return (tok->type == JSMN_PRIMITIVE && (*c == 't' || *c == 'f'));
}

static bool to_bool(char *data, jsmntok_t const *tok)
{
    char *c = data + tok->start;
    return *c == 't';
}

static enum rc bind_int64(char *data, jsmntok_t const *tok, int64_t *val)
{
    if (!is_number(data, tok)) return einval("expected number");
    if (!to_int64l(tokl(tok), tokv(data, tok), val))
        return eparse("parse number");
    return RC_OK;
}

static void cpy_str(unsigned dst_sz, char *dst, char *src, jsmntok_t const *tok)
{
    unsigned len = xmath_min(dst_sz - 1, tokl(tok));
    memcpy(dst, tokv(src, tok), len);
    dst[len] = 0;
}

static enum rc parse_job(char *data, unsigned ntoks, jsmntok_t const *tok,
                         struct sched_job *job)
{
    if (ntoks != 2 * 9 + 1) return error(RC_EINVAL, "expected nine items");

    enum rc rc = RC_OK;
    for (unsigned i = 1; i < ntoks; i += 2)
    {
        if (jeq(data, tok + i, "id"))
        {
            if ((rc = bind_int64(data, tok + i + 1, &job->id))) return rc;
        }
        else if (jeq(data, tok + i, "db_id"))
        {
            if ((rc = bind_int64(data, tok + i + 1, &job->db_id))) return rc;
        }
        else if (jeq(data, tok + i, "multi_hits"))
        {
            if (!is_boolean(data, tok + i + 1))
                return error(RC_EINVAL, "expected bool");
            job->multi_hits = to_bool(data, tok + i + 1);
        }
        else if (jeq(data, tok + i, "hmmer3_compat"))
        {
            if (!is_boolean(data, tok + i + 1))
                return error(RC_EINVAL, "expected bool");
            job->hmmer3_compat = to_bool(data, tok + i + 1);
        }
        else if (jeq(data, tok + i, "state"))
        {
            cpy_str(JOB_STATE_SIZE, job->state, data, tok + i + 1);
        }
        else if (jeq(data, tok + i, "error"))
        {
            cpy_str(JOB_ERROR_SIZE, job->error, data, tok + i + 1);
        }
        else if (jeq(data, tok + i, "submission"))
        {
            if ((rc = bind_int64(data, tok + i + 1, &job->submission)))
                return rc;
        }
        else if (jeq(data, tok + i, "exec_started"))
        {
            if ((rc = bind_int64(data, tok + i + 1, &job->exec_started)))
                return rc;
        }
        else if (jeq(data, tok + i, "exec_ended"))
        {
            if ((rc = bind_int64(data, tok + i + 1, &job->exec_ended)))
                return rc;
        }
        else
            return error(RC_EINVAL, "unexpected json key");
    }
    return RC_OK;
}

static size_t next_pend_job_callback(void *contents, size_t bsize, size_t nmemb,
                                     void *userp)
{
    struct sched_job *job = userp;
    (void)userp;
    size_t size = bsize * nmemb;
    char *data = contents;

    jsmn_init(&json.parser);
    int r = jsmn_parse(&json.parser, data, size, json.tok, 128);
    if (r < 0)
    {
        return 0;
    }
    if (r < 1 || json.tok[0].type != JSMN_ARRAY)
    {
        return 0;
    }

    json.ntoks = (unsigned)(r - 1);
    if (parse_job(data, json.ntoks, json.tok + 1, job)) return 0;
    return size;
}

static size_t get_db_callback(void *contents, size_t size, size_t nmemb,
                              void *userp)
{
}

enum rc rest_open(char const *url_stem)
{
    spinlock_lock(&lock);
    enum rc rc = rest_curl_init(&curl, url_stem);
    spinlock_unlock(&lock);
    return rc;
}

void rest_close(void)
{
    spinlock_lock(&lock);
    rest_curl_del(&curl);
    spinlock_unlock(&lock);
}

enum rc rest_next_pend_job(struct sched_job *job)
{
    enum rc rc = rest_curl_http_get(&curl, "/jobs/next_pend",
                                    next_pend_job_callback, job);
    //
    //
    // reset_curl_handle();
    // rest_url_set_query(&rest_url, "/jobs/next_pend");
    //
    // CURLcode code = curl_easy_setopt(curl.handle, CURLOPT_URL,
    // rest_url.full);
    //
    // code = curl_easy_perform(curl.handle);
    // // CURLE_COULDNT_CONNECT
    // if (curl_easy_perform(curl.handle) != CURLE_OK)
    //     return efail("curl_easy_perform");
    //
    // long http_code = 0;
    // curl_easy_getinfo(curl.handle, CURLINFO_RESPONSE_CODE, &http_code);
    // // TODO: chek for the real answer
    // if (http_code == 404) return RC_END;
    //
    // printf("%.*s\n", (int)answer.size, answer.data);
    // return parse_job(answer.json.ntoks, answer.json.tok, job);
    return rc;
}

enum rc rest_post_db(struct sched_db *db)
{
    char json[FILENAME_SIZE + 16] = {0};
    sprintf(json, "{\"filename\": \"%s\"}", db->filename);
    enum rc rc = rest_curl_http_post(&curl, "/dbs/", json);
    //
    //
    // reset_curl_handle();
    // rest_url_set_query(&rest_url, "/jobs/next_pend");
    //
    // CURLcode code = curl_easy_setopt(curl.handle, CURLOPT_URL,
    // rest_url.full);
    //
    // code = curl_easy_perform(curl.handle);
    // // CURLE_COULDNT_CONNECT
    // if (curl_easy_perform(curl.handle) != CURLE_OK)
    //     return efail("curl_easy_perform");
    //
    // long http_code = 0;
    // curl_easy_getinfo(curl.handle, CURLINFO_RESPONSE_CODE, &http_code);
    // // TODO: chek for the real answer
    // if (http_code == 404) return RC_END;
    //
    // printf("%.*s\n", (int)answer.size, answer.data);
    // return parse_job(answer.json.ntoks, answer.json.tok, job);
    return rc;
}

enum rc rest_get_db(struct sched_db *db)
{
    char query[32] = {0};
    sprintf(query, "/dbs/%" PRId64, db->id);
    enum rc rc = rest_curl_http_get(&curl, query, get_db_callback, db);
    //
    //
    // reset_curl_handle();
    // rest_url_set_query(&rest_url, "/jobs/next_pend");
    //
    // CURLcode code = curl_easy_setopt(curl.handle, CURLOPT_URL,
    // rest_url.full);
    //
    // code = curl_easy_perform(curl.handle);
    // // CURLE_COULDNT_CONNECT
    // if (curl_easy_perform(curl.handle) != CURLE_OK)
    //     return efail("curl_easy_perform");
    //
    // long http_code = 0;
    // curl_easy_getinfo(curl.handle, CURLINFO_RESPONSE_CODE, &http_code);
    // // TODO: chek for the real answer
    // if (http_code == 404) return RC_END;
    //
    // printf("%.*s\n", (int)answer.size, answer.data);
    // return parse_job(answer.json.ntoks, answer.json.tok, job);
    return rc;
}
