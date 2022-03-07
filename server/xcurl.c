#include "deciphon/server/xcurl.h"
#include "curl_error.h"
#include "deciphon/logger.h"

static atomic_bool initialized = false;

enum rc xcurl_init(struct xcurl *x, char const *url_stem)
{
    enum rc rc = RC_OK;

    x->lock = SPINLOCK_INIT;
    initialized = false;
    x->headers.get = 0;
    x->headers.post = 0;
    x->headers.delete = 0;

    assert(!initialized);

    if (!initialized && curl_global_init(CURL_GLOBAL_ALL))
    {
        rc = efail("failed to initialized curl");
        goto cleanup;
    }
    initialized = true;

    struct curl_slist *h = x->headers.get;
    x->headers.get = curl_slist_append(h, "Accept: application/json");

    h = x->headers.post;
    x->headers.post = curl_slist_append(h, "Accept: application/json");
    x->headers.post = curl_slist_append(h, "Content-Type: application/json");

    h = x->headers.delete;
    x->headers.delete = curl_slist_append(h, "Accept: application/json");

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
    if (x->headers.get) curl_slist_free_all(x->headers.get);
    if (x->headers.post) curl_slist_free_all(x->headers.post);
    if (x->headers.delete) curl_slist_free_all(x->headers.delete);
    x->headers.get = 0;
    x->headers.post = 0;
    x->headers.delete = 0;
    if (initialized) curl_global_cleanup();
    initialized = false;
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

static void setup_common_options(CURL *curl, char const *url,
                                 xcurl_callback_func_t callback, void *arg)
{
    curl_easy_reset(curl);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback_func);
    struct callback_data cd = {callback, arg};
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &cd);
    curl_easy_setopt(curl, CURLOPT_URL, url);
}

static enum rc perform_request(CURL *curl, long *http_code)
{
    CURLcode code = curl_easy_perform(curl);
    if (code) return curl_error(code);

    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, http_code);
    return *http_code > 299 ? efail("http request error") : RC_OK;
}

enum rc xcurl_http_get(struct xcurl *x, char const *query, long *http_code,
                       xcurl_callback_func_t callback, void *arg)
{
    url_set_query(&x->url, query);
    setup_common_options(x->curl, x->url.full, callback, arg);

    curl_easy_setopt(x->curl, CURLOPT_HTTPHEADER, x->headers.get);
    curl_easy_setopt(x->curl, CURLOPT_HTTPGET, 1L);

    return perform_request(x->curl, http_code);
}

enum rc xcurl_http_post(struct xcurl *x, char const *query, long *http_code,
                        xcurl_callback_func_t callback, void *arg,
                        char const *json)
{
    url_set_query(&x->url, query);
    setup_common_options(x->curl, x->url.full, callback, arg);

    curl_easy_setopt(x->curl, CURLOPT_HTTPHEADER, x->headers.post);
    curl_easy_setopt(x->curl, CURLOPT_POST, 1L);
    curl_easy_setopt(x->curl, CURLOPT_POSTFIELDS, json);

    return perform_request(x->curl, http_code);
}

enum rc xcurl_http_delete(struct xcurl *x, char const *query, long *http_code,
                          xcurl_callback_func_t callback, void *arg)
{
    url_set_query(&x->url, query);
    setup_common_options(x->curl, x->url.full, callback, arg);

    curl_easy_setopt(x->curl, CURLOPT_HTTPHEADER, x->headers.delete);
    curl_easy_setopt(x->curl, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(x->curl, CURLOPT_CUSTOMREQUEST, "DELETE");

    return perform_request(x->curl, http_code);
}
