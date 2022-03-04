#include "deciphon/compiler.h"
#include "deciphon/rc.h"
#include "deciphon/server/server.h"
#include "deciphon/util/util.h"
#include "jsmn.h"
#include "rest_url.h"
#include "sched.h"
#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static char const job_states[][5] = {[SCHED_JOB_PEND] = "pend",
                                     [SCHED_JOB_RUN] = "run",
                                     [SCHED_JOB_DONE] = "done",
                                     [SCHED_JOB_FAIL] = "fail"};

struct rest_url rest_url = {0};

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

static struct
{
    CURL *handle;
    atomic_flag lock;
    atomic_bool initialized;

    struct curl_slist *headers;
} curl = {0, ATOMIC_FLAG_INIT, false, 0};

struct rest_ret rest_ret = {0};

struct rest_job_state job_state = {0};

static inline void lock(void)
{
    while (atomic_flag_test_and_set(&curl.lock))
        /* spin until the lock is acquired */;
}

static inline void unlock(void) { atomic_flag_clear(&curl.lock); }

static size_t answer_callback(void *contents, size_t size, size_t nmemb,
                              void *userp)
{
    (void)userp;
    struct answer *a = (struct answer *)&answer;
    a->data = contents;
    a->size = size * nmemb;

    // char *ptr = realloc(a->data, a->size + realsize + 1);
    // if (!ptr)
    // {
    //     error(DECIPHON_ENOMEM, "failed to realloc");
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
// curl_easy_reset

static void reset_curl_handle(void)
{
    curl_easy_reset(curl.handle);
    curl_easy_setopt(curl.handle, CURLOPT_WRITEFUNCTION, answer_callback);
    curl_easy_setopt(curl.handle, CURLOPT_WRITEDATA, 0);
    curl_easy_setopt(curl.handle, CURLOPT_HTTPHEADER, curl.headers);
}

enum rc rest_open(char const *url_stem)
{
    enum rc rc = RC_OK;

    if (!curl.initialized && curl_global_init(CURL_GLOBAL_ALL))
    {
        rc = efail("failed to initialized curl");
        goto cleanup;
    }
    curl.initialized = true;
    curl.headers = curl_slist_append(curl.headers, "Accept: application/json");
    if (!curl.headers)
    {
        rc = efail("failed to append header");
        goto cleanup;
    }

    if (!rest_url_init(&rest_url, url_stem))
    {
        rc = einval("too-long url stem");
        goto cleanup;
    }

    answer.data = malloc(1024);
    if (!answer.data)
    {
        rc = enomem("failed to allocate");
        goto cleanup;
    }
    answer.size = 0;

    if (!(curl.handle = curl_easy_init()))
    {
        rc = efail("curl_easy_init");
        goto cleanup;
    }

    reset_curl_handle();

    return RC_OK;

cleanup:
    rest_close();
    return rc;
}

void rest_close(void)
{
    if (curl.handle) curl_easy_cleanup(curl.handle);
    if (answer.data) free(answer.data);
    if (curl.initialized) curl_global_cleanup();

    curl.initialized = false;
    answer.data = 0;
    curl.handle = 0;

    curl_slist_free_all(curl.headers);
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

static enum rc bind_int64(jsmntok_t const *tok, int64_t *val)
{
    if (!is_number(tok)) return error(RC_EINVAL, "expected number");
    if (!to_int64l(tokl(tok), tokv(tok), val)) return eparse("parse number");
    return RC_OK;
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
    return resolve_rc(tokl(tok), tokv(tok), rc);
}

static enum rc parse_ret(unsigned nitems, jsmntok_t const *tok)
{
    if (nitems != 2) return error(RC_EINVAL, "expected two items");

    enum rc rc = RC_OK;
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
    return RC_OK;
}

static enum rc parse_db(unsigned ntoks, jsmntok_t const *tok,
                        struct sched_db *db)
{
    if (ntoks != 2 * 3 + 1) return error(RC_EINVAL, "expected three items");

    enum rc rc = RC_OK;
    for (unsigned i = 1; i < ntoks; i += 2)
    {
        if (jeq(tok + i, "id"))
        {
            if ((rc = bind_int64(tok + i + 1, &db->id))) return rc;
        }
        else if (jeq(tok + i, "xxh64"))
        {
            if ((rc = bind_int64(tok + i + 1, &db->xxh64))) return rc;
        }
        else if (jeq(tok + i, "filename"))
        {
            cpy_str(PATH_SIZE, db->filename, tok + i + 1);
        }
        else
            return error(RC_EINVAL, "unexpected json key");
    }
    return RC_OK;
}

#if 0
static enum rc parse_filepath(unsigned ntoks, jsmntok_t const *tok,
                              unsigned path_size, char *path)
{
    enum rc rc = RC_OK;
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
    return RC_OK;
}
#endif

// enum rc rest_get_db_filepath(unsigned path_size, char *path, int64_t id)
// {
//     sprintf(rest_url, "%s/db/filepath?db_id=%" PRId64, base_url, id);
//     curl_easy_setopt(curl, CURLOPT_URL, rest_url);
//
//     if (curl_easy_perform(curl) != CURLE_OK) return
//     efail("curl_easy_perform");
//
//     printf("%.*s\n", (int)answer.size, answer.data);
//     return parse_filepath(answer.json.ntoks, answer.json.tok, path_size,
//     path);
// }

#if 0
enum rc rest_get_db(struct sched_db *db)
{
    reset_curl_handle();
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Accept: application/json");
    curl_easy_setopt(curl.handle, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl.handle, CURLOPT_HTTPGET, 1L);

    sprintf(rest_url, "%s/dbs/%" PRId64, url_stem, db->id);
    curl_easy_setopt(curl, CURLOPT_URL, rest_url);

    if (curl_easy_perform(curl) != CURLE_OK) return efail("curl_easy_perform");

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    if (http_code == 404) return error(RC_END, "db not found");

    printf("%.*s\n", (int)answer.size, answer.data);
    curl_slist_free_all(headers);
    return parse_db(answer.json.ntoks, answer.json.tok, db);
}
#endif

// static enum rc parse_job_state(void)
// {
//     struct answer *a = (struct answer *)&answer;
//     enum rc rc = RC_OK;
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
//     return RC_OK;
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
//     return RC_OK;
// }

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
    return RC_OK;
}

static enum rc parse_next_pend_job(unsigned ntoks, jsmntok_t const *tok,
                                   struct rest_pend_job *job)
{
    enum rc rc = RC_OK;
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
    return RC_OK;
}

static enum rc parse_job(unsigned ntoks, jsmntok_t const *tok,
                         struct sched_job *job)
{
    if (ntoks != 2 * 9 + 1) return error(RC_EINVAL, "expected nine items");

    enum rc rc = RC_OK;
    for (unsigned i = 1; i < ntoks; i += 2)
    {
        if (jeq(tok + i, "id"))
        {
            if ((rc = bind_int64(tok + i + 1, &job->id))) return rc;
        }
        else if (jeq(tok + i, "db_id"))
        {
            if ((rc = bind_int64(tok + i + 1, &job->db_id))) return rc;
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
        else if (jeq(tok + i, "state"))
        {
            cpy_str(JOB_STATE_SIZE, job->state, tok + i + 1);
        }
        else if (jeq(tok + i, "error"))
        {
            cpy_str(JOB_ERROR_SIZE, job->error, tok + i + 1);
        }
        else if (jeq(tok + i, "submission"))
        {
            if ((rc = bind_int64(tok + i + 1, &job->submission))) return rc;
        }
        else if (jeq(tok + i, "exec_started"))
        {
            if ((rc = bind_int64(tok + i + 1, &job->exec_started))) return rc;
        }
        else if (jeq(tok + i, "exec_ended"))
        {
            if ((rc = bind_int64(tok + i + 1, &job->exec_ended))) return rc;
        }
        else
            return error(RC_EINVAL, "unexpected json key");
    }
    return RC_OK;
}

#if 0
enum rc rest_set_job_state(struct sched_job *job, enum sched_job_state state,
                           char const *error)
{
    reset_curl_handle();
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Accept: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PATCH");

    char *eerr = curl_easy_escape(curl, error, 0);

    sprintf(rest_url, "%s/jobs/%" PRId64 "?state=%s&error=%s", url_stem,
            job->id, job_states[state], eerr);
    curl_easy_setopt(curl, CURLOPT_URL, rest_url);

    if (curl_easy_perform(curl) != CURLE_OK) return efail("curl_easy_perform");

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    if (http_code == 403) return error(RC_EINVAL, "redundant job state update");
    if (http_code == 500) return error(RC_EFAIL, "server error");

    printf("%.*s\n", (int)answer.size, answer.data);
    curl_slist_free_all(headers);
    curl_free(eerr);
    return parse_job(answer.json.ntoks, answer.json.tok, job);
}
#endif

enum rc rest_next_pend_job(struct sched_job *job)
{
    reset_curl_handle();
    rest_url_set_query(&rest_url, "/jobs/next_pend");

    CURLcode code = curl_easy_setopt(curl.handle, CURLOPT_URL, rest_url.full);

    code = curl_easy_perform(curl.handle);
    // CURLE_COULDNT_CONNECT
    if (curl_easy_perform(curl.handle) != CURLE_OK)
        return efail("curl_easy_perform");

    long http_code = 0;
    curl_easy_getinfo(curl.handle, CURLINFO_RESPONSE_CODE, &http_code);
    // TODO: chek for the real answer
    if (http_code == 404) return RC_END;

    printf("%.*s\n", (int)answer.size, answer.data);
    return parse_job(answer.json.ntoks, answer.json.tok, job);
}

static enum rc parse_seq(unsigned nitems, jsmntok_t const *tok,
                         struct sched_seq *seq)
{
    if (nitems != 4 * 2 + 1) return error(RC_EINVAL, "expected five items");

    enum rc rc = RC_OK;
    for (unsigned i = 1; i < nitems; i += 2)
    {
        if (jeq(tok + i, "id"))
        {
            if ((rc = bind_int64(tok + i + 1, &seq->id))) return rc;
        }
        else if (jeq(tok + i, "job_id"))
        {
            if ((rc = bind_int64(tok + i + 1, &seq->job_id))) return rc;
        }
        else if (jeq(tok + i, "name"))
        {
            cpy_str(SEQ_NAME_SIZE, seq->name, tok + i + 1);
        }
        else if (jeq(tok + i, "data"))
        {
            cpy_str(SEQ_SIZE, seq->data, tok + i + 1);
        }
        else
            return error(RC_EINVAL, "unexpected json key");
    }
    return RC_OK;
}

#if 0
static enum rc parse_next_seq(unsigned ntoks, jsmntok_t const *tok,
                              struct sched_seq *seq)
{
    enum rc rc = RC_OK;
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
    return RC_OK;
}
#endif

// HEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEREEEEEEEEEEEEEEEEEEEEEEEEEEEE
// long http_code = 0;
// curl_easy_getinfo (session, CURLINFO_RESPONSE_CODE, &http_code);

#if 0
enum rc rest_next_seq(struct sched_seq *seq)
{
    reset_curl_handle();
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Accept: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);

    sprintf(rest_url, "%s/jobs/%" PRId64 "/seqs/next/%" PRId64, url_stem,
            seq->job_id, seq->id);
    curl_easy_setopt(curl, CURLOPT_URL, rest_url);

    if (curl_easy_perform(curl) != CURLE_OK) return efail("curl_easy_perform");

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    if (http_code == 404) return RC_END;
    if (http_code == 500) return error(RC_EFAIL, "server error");

    printf("%.*s\n", (int)answer.size, answer.data);
    curl_slist_free_all(headers);
    return parse_seq(answer.json.ntoks, answer.json.tok, seq);
}
#endif

static void dump(const char *text, FILE *stream, unsigned char *ptr,
                 size_t size)
{
    size_t i;
    size_t c;
    unsigned int width = 0x30;

    fprintf(stream, "%s, %10.10ld bytes (0x%8.8lx)\n", text, (long)size,
            (long)size);

    for (i = 0; i < size; i += width)
    {
        fprintf(stream, "%4.4lx: ", (long)i);

#if 0
        /* show hex to the left */
        for (c = 0; c < width; c++)
        {
            if (i + c < size)
                fprintf(stream, "%02x ", ptr[i + c]);
            else
                fputs("   ", stream);
        }
#endif

        /* show data on the right */
        for (c = 0; (c < width) && (i + c < size); c++)
        {
            char x =
                (ptr[i + c] >= 0x20 && ptr[i + c] < 0x80) ? ptr[i + c] : '.';
            fputc(x, stream);
        }

        fputc('\n', stream); /* newline */
    }
}

static int my_trace(CURL *handle, curl_infotype type, char *data, size_t size,
                    void *userp)
{
    const char *text;
    (void)handle; /* prevent compiler warning */
    (void)userp;

    switch (type)
    {
    case CURLINFO_TEXT:
        fprintf(stderr, "== Info: %s", data);
    default: /* in case a new one is introduced to shock us */
        return 0;

    case CURLINFO_HEADER_OUT:
        text = "=> Send header";
        break;
    case CURLINFO_DATA_OUT:
        text = "=> Send data";
        break;
    case CURLINFO_SSL_DATA_OUT:
        text = "=> Send SSL data";
        break;
    case CURLINFO_HEADER_IN:
        text = "<= Recv header";
        break;
    case CURLINFO_DATA_IN:
        text = "<= Recv data";
        break;
    case CURLINFO_SSL_DATA_IN:
        text = "<= Recv SSL data";
        break;
    }

    dump(text, stderr, (unsigned char *)data, size);
    return 0;
}

#if 0
enum rc rest_submit_prods_file(char const *filepath)
{
    // struct stat file_info = {0};
    /* to get the file size */
    // if (fstat(fileno(fp), &file_info) != 0) return eio("fstat");

    reset_curl_handle();
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Accept: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    // curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, my_trace);
    // curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

    /* set where to read from (on Windows you need to use READFUNCTION too) */
    // curl_easy_setopt(curl, CURLOPT_READDATA, fp);

    /* and give the size of the upload (optional) */
    // curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE,
    //                  (curl_off_t)file_info.st_size);

    curl_mime *form = NULL;
    curl_mimepart *field = NULL;
    form = curl_mime_init(curl);

    field = curl_mime_addpart(form);
    curl_mime_name(field, "name");
    curl_mime_data(field, "prods_file", CURL_ZERO_TERMINATED);

    field = curl_mime_addpart(form);
    curl_mime_name(field, "prods_file");
    curl_mime_filedata(field, filepath);
    curl_mime_type(field, "text/tab-separated-values");

    field = curl_mime_addpart(form);
    curl_mime_name(field, "filename");
    curl_mime_data(field, "prods.tsv", CURL_ZERO_TERMINATED);

    sprintf(rest_url, "%s/prods/upload", url_stem);
    curl_easy_setopt(curl, CURLOPT_URL, rest_url);
    curl_easy_setopt(curl, CURLOPT_MIMEPOST, form);

    if (curl_easy_perform(curl) != CURLE_OK)
    {
        curl_mime_free(form);
        curl_slist_free_all(headers);
        return efail("curl_easy_perform");
    }
    // curl_mime_free(form);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    curl_mime_free(form);
    curl_slist_free_all(headers);
    if (http_code != 201) return efail("upload file");

    return RC_OK;
}
#endif
