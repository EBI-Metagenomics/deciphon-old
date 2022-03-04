#include "deciphon/server/rest_curl.h"
#include "curl_error.h"
#include "deciphon/util/util.h"

static atomic_bool initialized = false;

enum rc rest_curl_init(struct rest_curl *curl, char const *url_stem)
{
    enum rc rc = RC_OK;

    curl->lock = SPINLOCK_INIT;
    initialized = false;
    curl->headers.get = 0;
    curl->headers.post = 0;

    assert(!initialized);

    if (!initialized && curl_global_init(CURL_GLOBAL_ALL))
    {
        rc = efail("failed to initialized curl");
        goto cleanup;
    }
    initialized = true;
    curl->headers.get =
        curl_slist_append(curl->headers.get, "Accept: application/json");

    curl->headers.post =
        curl_slist_append(curl->headers.post, "Accept: application/json");
    curl->headers.post =
        curl_slist_append(curl->headers.post, "Content-Type: application/json");

    if (!(curl->handle = curl_easy_init()))
    {
        rc = efail("failed to initialize curl");
        goto cleanup;
    }

    if (!rest_url_init(&curl->url, url_stem))
    {
        rc = einval("invalid url stem");
        goto cleanup;
    }

    return rc;

cleanup:
    rest_curl_del(curl);
    return rc;
}

static void rest_curl_reset(struct rest_curl *curl, struct curl_slist *headers)
{
    curl_easy_reset(curl->handle);
    curl_easy_setopt(curl->handle, CURLOPT_HTTPHEADER, headers);
}

void rest_curl_del(struct rest_curl *curl)
{
    if (curl->handle) curl_easy_cleanup(curl->handle);
    curl_slist_free_all(curl->headers.get);
    curl_slist_free_all(curl->headers.post);
    curl->headers.get = 0;
    curl->headers.post = 0;
    if (initialized) curl_global_cleanup();
    initialized = false;
}

static inline void set_query(struct rest_curl *curl, char const *query)
{
    rest_url_set_query(&curl->url, query);
    CURLcode code = curl_easy_setopt(curl->handle, CURLOPT_URL, curl->url.full);
    assert(!code);
}

enum rc rest_curl_http_get(struct rest_curl *curl, char const *query,
                           rest_curl_callback_func_t callback, void *arg)
{
    rest_curl_reset(curl, curl->headers.get);
    curl_easy_setopt(curl->handle, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(curl->handle, CURLOPT_WRITEFUNCTION, callback);
    curl_easy_setopt(curl->handle, CURLOPT_WRITEDATA, arg);
    set_query(curl, query);

    CURLcode code = curl_easy_perform(curl->handle);
    if (code) return curl_error(code);

    long http_code = 0;
    curl_easy_getinfo(curl->handle, CURLINFO_RESPONSE_CODE, &http_code);
    printf("HTTP code: %ld\n", http_code);
    return RC_OK;
}

enum rc rest_curl_http_post(struct rest_curl *curl, char const *query,
                            char const *json)
{
    rest_curl_reset(curl, curl->headers.post);
    curl_easy_setopt(curl->handle, CURLOPT_POST, 1L);
    curl_easy_setopt(curl->handle, CURLOPT_POSTFIELDS, json);
    set_query(curl, query);

    CURLcode code = curl_easy_perform(curl->handle);
    if (code) return curl_error(code);

    long http_code = 0;
    curl_easy_getinfo(curl->handle, CURLINFO_RESPONSE_CODE, &http_code);
    printf("HTTP code: %ld\n", http_code);
    return RC_OK;
}
