#include "rest.h"
#include "common/compiler.h"
#include "common/jsmn.h"
#include "common/logger.h"
#include "common/rc.h"
#include "common/xmath.h"
#include "sched/job.h"
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

static inline unsigned json_toklen(unsigned i)
{
    struct answer const *a = (struct answer *)&answer;
    return (unsigned)(a->json.tok[i].end - a->json.tok[i].start);
}

static inline char const *json_tok(unsigned i)
{
    struct answer const *a = (struct answer *)&answer;
    return a->data + a->json.tok[i].start;
}

static void parse_job_state(void)
{
    struct answer *a = (struct answer *)&answer;
    for (unsigned i = 1; i < (unsigned)a->json.r; i++)
    {
        if (jsoneq(a->data, &a->json.tok[i], "rc") == 0)
        {
            unsigned len = json_toklen(i + 1);
            rc_from_str(len, json_tok(i + 1), &job_state.rc);
            i++;
        }
        else if (jsoneq(a->data, &a->json.tok[i], "error") == 0)
        {
            unsigned len = xmath_min(JOB_ERROR_SIZE - 1, json_toklen(i + 1));
            memcpy(job_state.error, json_tok(i + 1), len);
            job_state.error[len] = 0;
            i++;
        }
        else if (jsoneq(a->data, &a->json.tok[i], "state") == 0)
        {
            unsigned len = xmath_min(JOB_STATE_SIZE - 1, json_toklen(i + 1));
            memcpy(job_state.state, json_tok(i + 1), len);
            job_state.state[len] = 0;
            i++;
        }
        else
        {
            printf("Unexpected key: %.*s\n", json_toklen(i), json_tok(i));
        }
    }
}

char url[1024] = {0};

enum rc rest_job_state(int64_t job_id)
{
    struct answer *a = (struct answer *)&answer;
    jsmn_init(&a->json.parser);
    a->data = malloc(1); /* will be grown as needed by the realloc above */
    a->size = 0;         /* no data at this point */
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

        printf("%lu bytes retrieved\n", (unsigned long)a->size);
        printf("%s\n", a->data);
        parse_job_state();
        printf("%d\n", job_state.rc);
        printf("%s\n", job_state.error);
        printf("%s\n", job_state.state);
    }

    curl_easy_cleanup(curl);
    free(a->data);

    /* we are done with libcurl, so clean it up */
    curl_global_cleanup();
    return RC_DONE;
}
