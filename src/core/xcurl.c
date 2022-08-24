#include "deciphon/core/xcurl.h"
#include "core/xcurl_header.h"
#include "core/xcurl_mime.h"
#include "deciphon/core/limits.h"
#include "deciphon/core/logging.h"
#include "xcurl_debug.h"
#include "xcurl_error.h"
#include <curl/curl.h>
#include <string.h>

static struct xcurl
{
    unsigned initialized;
    CURL *curl;
    struct
    {
        struct curl_slist *send_json;
        struct curl_slist *recv_json;
        struct curl_slist *only_json;
    } hdr;
    struct url url;
} xcurl = {0};

#define TIMEOUT 3000L
#define CONNECTTIMEOUT 5L

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

static char key_header[256 + 11] = "X-API-KEY: ";

static enum rc setup_headers(char const *api_key)
{
    xcurl.hdr.send_json = 0;
    xcurl.hdr.recv_json = 0;
    xcurl.hdr.only_json = 0;

    strcpy(key_header + strlen(key_header), api_key);

    enum rc rc = RC_OK;

    rc = list_add(&xcurl.hdr.send_json, "Content-Type: application/json");
    if (rc) goto cleanup;
    rc = list_add(&xcurl.hdr.send_json, key_header);
    if (rc) goto cleanup;

    rc = list_add(&xcurl.hdr.recv_json, "Accept: application/json");
    if (rc) goto cleanup;
    rc = list_add(&xcurl.hdr.recv_json, key_header);
    if (rc) goto cleanup;

    rc = list_add(&xcurl.hdr.only_json, "Content-Type: application/json");
    if (rc) goto cleanup;
    rc = list_add(&xcurl.hdr.only_json, "Accept: application/json");
    if (rc) goto cleanup;
    rc = list_add(&xcurl.hdr.only_json, key_header);
    if (rc) goto cleanup;

    return rc;

cleanup:
    list_free(xcurl.hdr.send_json);
    list_free(xcurl.hdr.recv_json);
    list_free(xcurl.hdr.only_json);
    return rc;
}

static size_t noop_write(void *ptr, size_t size, size_t nmemb, void *data)
{
    (void)ptr;
    (void)data;
    return size * nmemb;
}

enum rc xcurl_init(char const *url_stem, char const *api_key)
{
    if (xcurl.initialized++) return RC_OK;

    enum rc rc = RC_OK;

    if (curl_global_init(CURL_GLOBAL_DEFAULT))
    {
        rc = efail("failed to initialized curl");
        goto cleanup;
    }

    if ((rc = setup_headers(api_key))) goto cleanup;

    xcurl.curl = curl_easy_init();
    if (!xcurl.curl)
    {
        rc = efail("failed to initialize curl");
        goto cleanup;
    }

    if (!url_init(&xcurl.url, url_stem))
    {
        rc = einval("invalid url stem");
        goto cleanup;
    }

    return rc;

cleanup:
    xcurl_cleanup();
    return rc;
}

void xcurl_cleanup(void)
{
    if (!xcurl.initialized) return;
    if (--xcurl.initialized) return;

    if (xcurl.curl) curl_easy_cleanup(xcurl.curl);
    xcurl.curl = 0;
    list_free(xcurl.hdr.send_json);
    list_free(xcurl.hdr.recv_json);
    list_free(xcurl.hdr.only_json);
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
    if (code) return xcurl_error(code);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, http_code);

    curl_easy_reset(curl);
    return RC_OK;
}

enum rc xcurl_get(char const *query, long *http_code,
                  xcurl_callback_func_t callback, void *arg)
{
    url_set_query(&xcurl.url, query);
    curl_easy_setopt(xcurl.curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(xcurl.curl, CURLOPT_URL, xcurl.url.full);
    curl_easy_setopt(xcurl.curl, CURLOPT_CONNECTTIMEOUT, CONNECTTIMEOUT);

    curl_easy_setopt(xcurl.curl, CURLOPT_WRITEFUNCTION, callback_func);
    struct callback_data cd = {callback, arg};
    curl_easy_setopt(xcurl.curl, CURLOPT_WRITEDATA, &cd);

    curl_easy_setopt(xcurl.curl, CURLOPT_HTTPHEADER,
                     xcurl_header(key_header, "Accept: application/json"));
    curl_easy_setopt(xcurl.curl, CURLOPT_HTTPGET, 1L);

    return perform_request(xcurl.curl, http_code);
}

enum rc xcurl_post(char const *query, long *http_code,
                   xcurl_callback_func_t callback, void *arg, char const *json)
{
    url_set_query(&xcurl.url, query);
    curl_easy_setopt(xcurl.curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(xcurl.curl, CURLOPT_URL, xcurl.url.full);
    curl_easy_setopt(xcurl.curl, CURLOPT_CONNECTTIMEOUT, CONNECTTIMEOUT);

    curl_easy_setopt(xcurl.curl, CURLOPT_WRITEFUNCTION, callback_func);
    struct callback_data cd = {callback, arg};
    curl_easy_setopt(xcurl.curl, CURLOPT_WRITEDATA, &cd);

    curl_easy_setopt(xcurl.curl, CURLOPT_HTTPHEADER, xcurl.hdr.only_json);
    curl_easy_setopt(xcurl.curl, CURLOPT_POST, 1L);
    curl_easy_setopt(xcurl.curl, CURLOPT_POSTFIELDS, json);

    xcurl_debug_setup(xcurl.curl);

    return perform_request(xcurl.curl, http_code);
}

enum rc xcurl_patch(char const *query, long *http_code,
                    xcurl_callback_func_t callback, void *arg, char const *json)
{
    url_set_query(&xcurl.url, query);
    curl_easy_setopt(xcurl.curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(xcurl.curl, CURLOPT_URL, xcurl.url.full);
    curl_easy_setopt(xcurl.curl, CURLOPT_CONNECTTIMEOUT, CONNECTTIMEOUT);

    curl_easy_setopt(xcurl.curl, CURLOPT_WRITEFUNCTION, callback_func);
    struct callback_data cd = {callback, arg};
    curl_easy_setopt(xcurl.curl, CURLOPT_WRITEDATA, &cd);

    curl_easy_setopt(xcurl.curl, CURLOPT_HTTPHEADER, xcurl.hdr.only_json);
    curl_easy_setopt(xcurl.curl, CURLOPT_POSTFIELDS, json);
    curl_easy_setopt(xcurl.curl, CURLOPT_CUSTOMREQUEST, "PATCH");

    xcurl_debug_setup(xcurl.curl);

    return perform_request(xcurl.curl, http_code);
}

enum rc xcurl_delete(char const *query, long *http_code)
{
    url_set_query(&xcurl.url, query);
    curl_easy_setopt(xcurl.curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(xcurl.curl, CURLOPT_URL, xcurl.url.full);
    curl_easy_setopt(xcurl.curl, CURLOPT_CONNECTTIMEOUT, CONNECTTIMEOUT);

    curl_easy_setopt(xcurl.curl, CURLOPT_HTTPHEADER, xcurl.hdr.recv_json);
    curl_easy_setopt(xcurl.curl, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(xcurl.curl, CURLOPT_WRITEFUNCTION, noop_write);
    curl_easy_setopt(xcurl.curl, CURLOPT_CUSTOMREQUEST, "DELETE");

    xcurl_debug_setup(xcurl.curl);

    return perform_request(xcurl.curl, http_code);
}

enum rc xcurl_download(char const *query, long *http_code, FILE *fp)
{
    url_set_query(&xcurl.url, query);
    curl_easy_setopt(xcurl.curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(xcurl.curl, CURLOPT_URL, xcurl.url.full);
    curl_easy_setopt(xcurl.curl, CURLOPT_CONNECTTIMEOUT, CONNECTTIMEOUT);
    curl_easy_setopt(xcurl.curl, CURLOPT_TIMEOUT, TIMEOUT);

    curl_easy_setopt(xcurl.curl, CURLOPT_WRITEFUNCTION, 0);
    curl_easy_setopt(xcurl.curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(xcurl.curl, CURLOPT_HTTPGET, 1L);

    xcurl_debug_setup(xcurl.curl);

    return perform_request(xcurl.curl, http_code);
}

enum rc xcurl_upload(char const *query, long *http_code,
                     xcurl_callback_func_t callback, void *arg,
                     struct xcurl_mime_file const *mime, char const *filepath)
{
    url_set_query(&xcurl.url, query);
    curl_easy_setopt(xcurl.curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(xcurl.curl, CURLOPT_URL, xcurl.url.full);
    curl_easy_setopt(xcurl.curl, CURLOPT_CONNECTTIMEOUT, CONNECTTIMEOUT);
    curl_easy_setopt(xcurl.curl, CURLOPT_TIMEOUT, TIMEOUT);

    curl_easy_setopt(xcurl.curl, CURLOPT_WRITEFUNCTION, callback_func);
    struct callback_data cd = {callback, arg};
    curl_easy_setopt(xcurl.curl, CURLOPT_WRITEDATA, &cd);

    curl_easy_setopt(xcurl.curl, CURLOPT_HTTPHEADER, xcurl.hdr.recv_json);

    curl_mime *form = xcurl_mime_new_file(xcurl.curl, mime, filepath);
    if (!form) return enomem("failed to allocate for mime");
    curl_easy_setopt(xcurl.curl, CURLOPT_MIMEPOST, form);

    xcurl_debug_setup(xcurl.curl);

    enum rc rc = perform_request(xcurl.curl, http_code);
    xcurl_mime_del(form);

    return rc;
}
