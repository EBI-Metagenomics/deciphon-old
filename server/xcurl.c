#include "deciphon/server/xcurl.h"
#include "curl_error.h"
#include "deciphon/logger.h"
#include "xcurl_debug.h"

enum rc xcurl_init(struct xcurl *x, char const *url_stem)
{
    enum rc rc = RC_OK;

    x->headers = 0;

    if (curl_global_init(CURL_GLOBAL_ALL))
    {
        rc = efail("failed to initialized curl");
        goto cleanup;
    }

    struct curl_slist *h = x->headers;
    x->headers = curl_slist_append(h, "Content-Type: application/json");
    x->headers = curl_slist_append(h, "Accept: application/json");

    if (!(x->curl = curl_easy_init()))
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
    return *http_code > 299 ? efail("http request error") : RC_OK;
}

enum rc xcurl_get(struct xcurl *x, char const *query, long *http_code,
                  xcurl_callback_func_t callback, void *arg)
{
    url_set_query(&x->url, query);
    curl_easy_reset(&x->curl);
    curl_easy_setopt(&x->curl, CURLOPT_WRITEFUNCTION, callback_func);
    struct callback_data cd = {callback, arg};
    curl_easy_setopt(&x->curl, CURLOPT_WRITEDATA, &cd);
    curl_easy_setopt(&x->curl, CURLOPT_URL, x->url.full);

    curl_easy_setopt(x->curl, CURLOPT_HTTPHEADER, x->headers);
    curl_easy_setopt(x->curl, CURLOPT_HTTPGET, 1L);

    return perform_request(x->curl, http_code);
}

enum rc xcurl_post(struct xcurl *x, char const *query, long *http_code,
                   xcurl_callback_func_t callback, void *arg, char const *json)
{
    url_set_query(&x->url, query);
    curl_easy_reset(&x->curl);
    curl_easy_setopt(&x->curl, CURLOPT_WRITEFUNCTION, callback_func);
    struct callback_data cd = {callback, arg};
    curl_easy_setopt(&x->curl, CURLOPT_WRITEDATA, &cd);
    curl_easy_setopt(&x->curl, CURLOPT_URL, x->url.full);

    curl_easy_setopt(x->curl, CURLOPT_HTTPHEADER, x->headers);
    curl_easy_setopt(x->curl, CURLOPT_POST, 1L);
    curl_easy_setopt(x->curl, CURLOPT_POSTFIELDS, json);

    return perform_request(x->curl, http_code);
}

enum rc xcurl_delete(struct xcurl *x, char const *query, long *http_code)
{
    curl_easy_reset(&x->curl);
    url_set_query(&x->url, query);
    curl_easy_setopt(&x->curl, CURLOPT_URL, x->url.full);

    curl_easy_setopt(x->curl, CURLOPT_HTTPHEADER, x->headers);
    curl_easy_setopt(x->curl, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(x->curl, CURLOPT_CUSTOMREQUEST, "DELETE");

    xcurl_debug_setup(x->curl);

    return perform_request(x->curl, http_code);
}
