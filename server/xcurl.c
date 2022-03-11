#include "deciphon/server/xcurl.h"
#include "curl_error.h"
#include "deciphon/logger.h"
#include "xcurl_debug.h"

enum rc xcurl_init(struct xcurl *x, char const *url_stem)
{
    enum rc rc = RC_OK;
    x->headers = 0;

    if (curl_global_init(CURL_GLOBAL_DEFAULT))
    {
        rc = efail("failed to initialized curl");
        goto cleanup;
    }

    x->headers =
        curl_slist_append(x->headers, "Content-Type: application/json");
    x->headers = curl_slist_append(x->headers, "Accept: application/json");

    x->curl = curl_easy_init();
    if (!x->curl)
    {
        rc = efail("failed to initialize curl");
        goto cleanup;
    }

    if (!url_init(&x->url, url_stem))
    {
        rc = einval("invalid url stem");
        goto cleanup;
    }

    return rc;

cleanup:
    xcurl_del(x);
    return rc;
}

void xcurl_del(struct xcurl *x)
{
    if (x->curl) curl_easy_cleanup(x->curl);
    x->curl = 0;
    if (x->headers) curl_slist_free_all(x->headers);
    x->headers = 0;
    curl_global_cleanup();
}

struct callback_data
{
    xcurl_callback_func_t callback;
    void *arg;
};

static size_t callback_func(void *data, size_t one, size_t size, void *arg)
{
    (void)one;
    struct callback_data *cd = arg;
    return cd->callback(data, size, cd->arg);
}

static enum rc perform_request(CURL *curl, long *http_code)
{
    CURLcode code = curl_easy_perform(curl);
    if (code) return curl_error(code);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, http_code);

    curl_easy_reset(curl);
    return RC_OK;
}

enum rc xcurl_get(struct xcurl *x, char const *query, long *http_code,
                  xcurl_callback_func_t callback, void *arg)
{
    url_set_query(&x->url, query);
    curl_easy_setopt(x->curl, CURLOPT_WRITEFUNCTION, callback_func);
    struct callback_data cd = {callback, arg};
    curl_easy_setopt(x->curl, CURLOPT_WRITEDATA, &cd);
    curl_easy_setopt(x->curl, CURLOPT_URL, x->url.full);

    curl_easy_setopt(x->curl, CURLOPT_HTTPHEADER, x->headers);
    curl_easy_setopt(x->curl, CURLOPT_HTTPGET, 1L);

    return perform_request(x->curl, http_code);
}

enum rc xcurl_post(struct xcurl *x, char const *query, long *http_code,
                   xcurl_callback_func_t callback, void *arg, char const *json)
{
    url_set_query(&x->url, query);
    curl_easy_setopt(x->curl, CURLOPT_WRITEFUNCTION, callback_func);
    struct callback_data cd = {callback, arg};
    curl_easy_setopt(x->curl, CURLOPT_WRITEDATA, &cd);
    curl_easy_setopt(x->curl, CURLOPT_URL, x->url.full);

    curl_easy_setopt(x->curl, CURLOPT_HTTPHEADER, x->headers);
    curl_easy_setopt(x->curl, CURLOPT_POST, 1L);
    curl_easy_setopt(x->curl, CURLOPT_POSTFIELDS, json);

    xcurl_debug_setup(x->curl);

    return perform_request(x->curl, http_code);
}
// curl -X 'PATCH' \
//   'http://127.0.0.1:8000/jobs/1' \
//   -H 'accept: application/json' \
//   -H 'Content-Type: application/json' \
//   -d '{
//   "state": "run",
//   "error": ""
// }'

// content-disposition: attachment; filename="minifam.hmm"
// content-length: 271220
// content-type: application/octet-stream
// date: Fri,11 Mar 2022 10:53:23 GMT
// etag: 94ab2c4f0708d28c619f9e5c81a4dde5
// last-modified: Fri,11 Mar 2022 10:13:55 GMT
// server: uvicorn

enum rc xcurl_patch(struct xcurl *x, char const *query, long *http_code,
                    xcurl_callback_func_t callback, void *arg, char const *json)
{
    url_set_query(&x->url, query);
    curl_easy_setopt(x->curl, CURLOPT_WRITEFUNCTION, callback_func);
    struct callback_data cd = {callback, arg};
    curl_easy_setopt(x->curl, CURLOPT_WRITEDATA, &cd);
    curl_easy_setopt(x->curl, CURLOPT_URL, x->url.full);

    curl_easy_setopt(x->curl, CURLOPT_HTTPHEADER, x->headers);
    curl_easy_setopt(x->curl, CURLOPT_POSTFIELDS, json);
    curl_easy_setopt(x->curl, CURLOPT_CUSTOMREQUEST, "PATCH");

    xcurl_debug_setup(x->curl);

    return perform_request(x->curl, http_code);
}

enum rc xcurl_delete(struct xcurl *x, char const *query, long *http_code)
{
    url_set_query(&x->url, query);
    curl_easy_setopt(x->curl, CURLOPT_URL, x->url.full);

    curl_easy_setopt(x->curl, CURLOPT_HTTPHEADER, x->headers);
    curl_easy_setopt(x->curl, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(x->curl, CURLOPT_CUSTOMREQUEST, "DELETE");

    xcurl_debug_setup(x->curl);

    return perform_request(x->curl, http_code);
}

enum rc xcurl_download(struct xcurl *x, char const *query, long *http_code,
                       FILE *fp)
{
    url_set_query(&x->url, query);
    curl_easy_setopt(x->curl, CURLOPT_URL, x->url.full);
    curl_easy_setopt(x->curl, CURLOPT_WRITEFUNCTION, 0);
    curl_easy_setopt(x->curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(x->curl, CURLOPT_HTTPGET, 1L);

    xcurl_debug_setup(x->curl);

    return perform_request(x->curl, http_code);
}
