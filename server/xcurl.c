#include "deciphon/server/xcurl.h"
#include "curl_error.h"
#include "deciphon/logger.h"

static atomic_bool initialized = false;

enum rc xcurl_init(struct xcurl *curl, char const *url_stem)
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

    if (!url_init(&curl->url, url_stem))
    {
        rc = einval("invalid url stem");
        goto cleanup;
    }

    return rc;

cleanup:
    xcurl_del(curl);
    return rc;
}

static void rest_curl_reset(struct xcurl *curl, struct curl_slist *headers)
{
    curl_easy_reset(curl->handle);
    curl_easy_setopt(curl->handle, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl->handle, CURLOPT_CUSTOMREQUEST, NULL);
}

void xcurl_del(struct xcurl *curl)
{
    if (curl->handle) curl_easy_cleanup(curl->handle);
    curl_slist_free_all(curl->headers.get);
    curl_slist_free_all(curl->headers.post);
    curl->headers.get = 0;
    curl->headers.post = 0;
    if (initialized) curl_global_cleanup();
    initialized = false;
}

static inline void set_query(struct xcurl *curl, char const *query)
{
    url_set_query(&curl->url, query);
    CURLcode code = curl_easy_setopt(curl->handle, CURLOPT_URL, curl->url.full);
    assert(!code);
}

enum rc xcurl_http_get(struct xcurl *curl, char const *query, long *http_code,
                       xcurl_callback_func_t callback, void *arg)
{
    rest_curl_reset(curl, curl->headers.get);
    curl_easy_setopt(curl->handle, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(curl->handle, CURLOPT_WRITEFUNCTION, callback);
    curl_easy_setopt(curl->handle, CURLOPT_WRITEDATA, arg);
    set_query(curl, query);

    CURLcode code = curl_easy_perform(curl->handle);
    if (code) return curl_error(code);

    curl_easy_getinfo(curl->handle, CURLINFO_RESPONSE_CODE, http_code);
    return *http_code > 299 ? efail("http request error") : RC_OK;
}

enum rc xcurl_http_post(struct xcurl *curl, char const *query, long *http_code,
                        char const *json)
{
    rest_curl_reset(curl, curl->headers.post);
    curl_easy_setopt(curl->handle, CURLOPT_POST, 1L);
    curl_easy_setopt(curl->handle, CURLOPT_POSTFIELDS, json);
    set_query(curl, query);

    CURLcode code = curl_easy_perform(curl->handle);
    if (code) return curl_error(code);

    curl_easy_getinfo(curl->handle, CURLINFO_RESPONSE_CODE, http_code);
    return *http_code > 299 ? efail("http request error") : RC_OK;
}

enum rc xcurl_http_delete(struct xcurl *curl, char const *query,
                          long *http_code)
{
    rest_curl_reset(curl, curl->headers.get);
    curl_easy_setopt(curl->handle, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(curl->handle, CURLOPT_CUSTOMREQUEST, "DELETE");
    // curl_easy_setopt(curl->handle, CURLOPT_WRITEFUNCTION, callback);
    // curl_easy_setopt(curl->handle, CURLOPT_WRITEDATA, arg);
    set_query(curl, query);

    CURLcode code = curl_easy_perform(curl->handle);
    if (code) return curl_error(code);

    curl_easy_getinfo(curl->handle, CURLINFO_RESPONSE_CODE, http_code);
    return *http_code > 299 ? efail("http request error") : RC_OK;
}
