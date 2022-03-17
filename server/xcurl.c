#include "deciphon/server/xcurl.h"
#include "curl_error.h"
#include "deciphon/limits.h"
#include "deciphon/logger.h"
#include "deciphon/strlcpy.h"
#include "xcurl_debug.h"

void xcurl_mime_set(struct xcurl_mime *mime, char const *name,
                    char const *filename, char const *type)
{
    strlcpy(mime->name, name, FILENAME_SIZE);
    strlcpy(mime->filename, filename, FILENAME_SIZE);
    strlcpy(mime->type, type, MIME_TYPE_SIZE);
}

static inline enum rc list_add(struct curl_slist **list, const char *string)
{
    struct curl_slist *tmp = curl_slist_append(*list, string);
    if (!tmp)
    {
        curl_slist_free_all(*list);
        *list = 0;
        return enomem("failed to add header");
    }
    *list = tmp;
    return RC_OK;
}

static inline void list_free(struct curl_slist *list)
{
    curl_slist_free_all(list);
}

static enum rc setup_headers(struct xcurl *x)
{
    x->hdr.send_json = 0;
    x->hdr.recv_json = 0;
    x->hdr.only_json = 0;

    enum rc rc = RC_OK;

    rc = list_add(&x->hdr.send_json, "Content-Type: application/json");
    if (rc) goto cleanup;

    rc = list_add(&x->hdr.recv_json, "Accept: application/json");
    if (rc) goto cleanup;

    rc = list_add(&x->hdr.only_json, "Content-Type: application/json");
    if (rc) goto cleanup;

    rc = list_add(&x->hdr.only_json, "Accept: application/json");
    if (rc) goto cleanup;

    return rc;

cleanup:
    list_free(x->hdr.send_json);
    list_free(x->hdr.recv_json);
    list_free(x->hdr.only_json);
    return rc;
}

enum rc xcurl_init(struct xcurl *x, char const *url_stem)
{
    enum rc rc = RC_OK;

    if (curl_global_init(CURL_GLOBAL_DEFAULT))
    {
        rc = efail("failed to initialized curl");
        goto cleanup;
    }

    if ((rc = setup_headers(x))) goto cleanup;

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
    list_free(x->hdr.send_json);
    list_free(x->hdr.recv_json);
    list_free(x->hdr.only_json);
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

    curl_easy_setopt(x->curl, CURLOPT_HTTPHEADER, x->hdr.recv_json);
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

    curl_easy_setopt(x->curl, CURLOPT_HTTPHEADER, x->hdr.only_json);
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

    curl_easy_setopt(x->curl, CURLOPT_HTTPHEADER, x->hdr.only_json);
    curl_easy_setopt(x->curl, CURLOPT_POSTFIELDS, json);
    curl_easy_setopt(x->curl, CURLOPT_CUSTOMREQUEST, "PATCH");

    xcurl_debug_setup(x->curl);

    return perform_request(x->curl, http_code);
}

enum rc xcurl_delete(struct xcurl *x, char const *query, long *http_code)
{
    url_set_query(&x->url, query);
    curl_easy_setopt(x->curl, CURLOPT_URL, x->url.full);

    curl_easy_setopt(x->curl, CURLOPT_HTTPHEADER, x->hdr.recv_json);
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

enum rc xcurl_upload(struct xcurl *x, char const *query, long *http_code,
                     xcurl_callback_func_t callback, void *arg,
                     struct xcurl_mime const *mime, char const *filepath)
{
    url_set_query(&x->url, query);
    curl_easy_setopt(x->curl, CURLOPT_WRITEFUNCTION, callback_func);
    struct callback_data cd = {callback, arg};
    curl_easy_setopt(x->curl, CURLOPT_WRITEDATA, &cd);
    curl_easy_setopt(x->curl, CURLOPT_URL, x->url.full);

    curl_easy_setopt(x->curl, CURLOPT_HTTPHEADER, x->hdr.recv_json);

    curl_mime *form = curl_mime_init(x->curl);

    curl_mimepart *field = curl_mime_addpart(form);
    curl_mime_name(field, "name");
    curl_mime_data(field, mime->name, CURL_ZERO_TERMINATED);

    field = curl_mime_addpart(form);
    curl_mime_name(field, mime->name);
    curl_mime_type(field, mime->type);
    curl_mime_filedata(field, filepath);

    field = curl_mime_addpart(form);
    curl_mime_name(field, "filename");
    curl_mime_data(field, mime->filename, CURL_ZERO_TERMINATED);

    curl_easy_setopt(x->curl, CURLOPT_MIMEPOST, form);

    xcurl_debug_setup(x->curl);

    enum rc rc = perform_request(x->curl, http_code);
    curl_mime_free(form);
    return rc;
}
