#include "deciphon/core/xcurl.h"
#include "core/pp.h"
#include "core/xcurl_mime.h"
#include "deciphon/core/compiler.h"
#include "deciphon/core/limits.h"
#include "deciphon/core/logging.h"
#include "xcurl_debug.h"
#include "xcurl_error.h"
#include <assert.h>
#include <curl/curl.h>
#include <stdarg.h>
#include <string.h>

#define TIMEOUT 3000L
#define CONNECTTIMEOUT 5L

static char api_key_header[256 + 11] = "X-API-KEY: ";

static struct xcurl
{
    CURL *curl;
    struct
    {
        struct curl_slist *send_json;
        struct curl_slist *recv_json;
        struct curl_slist *only_json;
    } hdr;
    struct url url;
} xcurl = {0};

#define http_header(...) http_header_cnt(PP_NARG(__VA_ARGS__), __VA_ARGS__)
static struct curl_slist const *http_header_cnt(unsigned cnt, ...);

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

static enum rc setup_headers(char const *api_key)
{
    xcurl.hdr.send_json = 0;
    xcurl.hdr.recv_json = 0;
    xcurl.hdr.only_json = 0;

    strcpy(api_key_header + strlen(api_key_header), api_key);

    enum rc rc = RC_OK;

    rc = list_add(&xcurl.hdr.send_json, "Content-Type: application/json");
    if (rc) goto cleanup;
    rc = list_add(&xcurl.hdr.send_json, api_key_header);
    if (rc) goto cleanup;

    rc = list_add(&xcurl.hdr.recv_json, "Accept: application/json");
    if (rc) goto cleanup;
    rc = list_add(&xcurl.hdr.recv_json, api_key_header);
    if (rc) goto cleanup;

    rc = list_add(&xcurl.hdr.only_json, "Content-Type: application/json");
    if (rc) goto cleanup;
    rc = list_add(&xcurl.hdr.only_json, "Accept: application/json");
    if (rc) goto cleanup;
    rc = list_add(&xcurl.hdr.only_json, api_key_header);
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
    if (xcurl.curl) curl_easy_cleanup(xcurl.curl);
    xcurl.curl = 0;
    list_free(xcurl.hdr.send_json);
    list_free(xcurl.hdr.recv_json);
    list_free(xcurl.hdr.only_json);
    curl_global_cleanup();
}

struct callback_data
{
    xcurl_cb_t *callback;
    void *arg;
};

static size_t callback_func(void *data, size_t one, size_t size, void *arg)
{
    (void)one;
    struct callback_data *cd = arg;
    return (*cd->callback)(data, size, cd->arg);
}

static enum rc perform_request(CURL *curl, long *http_code)
{
    CURLcode code = curl_easy_perform(curl);
    if (code) return xcurl_error(code);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, http_code);

    curl_easy_reset(curl);
    return RC_OK;
}

enum rc xcurl_get(char const *query, long *http, xcurl_cb_t *callback,
                  void *arg)
{
    url_set_query(&xcurl.url, query);
    curl_easy_setopt(xcurl.curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(xcurl.curl, CURLOPT_URL, xcurl.url.full);
    curl_easy_setopt(xcurl.curl, CURLOPT_CONNECTTIMEOUT, CONNECTTIMEOUT);

    curl_easy_setopt(xcurl.curl, CURLOPT_WRITEFUNCTION, callback_func);
    struct callback_data cd = {callback, arg};
    curl_easy_setopt(xcurl.curl, CURLOPT_WRITEDATA, &cd);

    curl_easy_setopt(xcurl.curl, CURLOPT_HTTPHEADER,
                     http_header(api_key_header, "Accept: application/json"));
    curl_easy_setopt(xcurl.curl, CURLOPT_HTTPGET, 1L);

    return perform_request(xcurl.curl, http);
}

enum rc xcurl_post(char const *query, long *http_code, xcurl_cb_t *callback,
                   void *arg, char const *json)
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

enum rc xcurl_patch(char const *query, long *http_code, xcurl_cb_t *callback,
                    void *arg, char const *json)
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

enum rc xcurl_upload(char const *query, long *http_code, xcurl_cb_t *callback,
                     void *arg, struct xcurl_mime_file const *mime,
                     char const *filepath)
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

enum rc xcurl_upload2(char const *query, long *http_code, xcurl_cb_t *callback,
                      void *arg, int64_t db_id, bool multi_hits,
                      bool hmmer3_compat, struct xcurl_mime_file const *mime,
                      char const *filepath)
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

    curl_mime *form = curl_mime_init(xcurl.curl);

    static char db_id_str[18] = {0};
    sprintf(db_id_str, "%lld", db_id);
    curl_mimepart *field = curl_mime_addpart(form);
    curl_mime_name(field, "db_id");
    curl_mime_data(field, db_id_str, CURL_ZERO_TERMINATED);

    field = curl_mime_addpart(form);
    curl_mime_name(field, "multi_hits");
    curl_mime_data(field, multi_hits ? "True" : "False", CURL_ZERO_TERMINATED);

    field = curl_mime_addpart(form);
    curl_mime_name(field, "hmmer3_compat");
    curl_mime_data(field, hmmer3_compat ? "True" : "False",
                   CURL_ZERO_TERMINATED);

    field = curl_mime_addpart(form);
    curl_mime_name(field, "name");
    curl_mime_data(field, mime->name, CURL_ZERO_TERMINATED);
    field = curl_mime_addpart(form);
    curl_mime_name(field, mime->name);
    curl_mime_type(field, mime->type);
    curl_mime_filedata(field, filepath);
    field = curl_mime_addpart(form);
    curl_mime_name(field, "filename");
    curl_mime_data(field, mime->filename, CURL_ZERO_TERMINATED);

    if (!form) return enomem("failed to allocate for mime");
    curl_easy_setopt(xcurl.curl, CURLOPT_MIMEPOST, form);

    xcurl_debug_setup(xcurl.curl);

    enum rc rc = perform_request(xcurl.curl, http_code);
    xcurl_mime_del(form);

    return rc;
}

static struct curl_slist const *http_header_cnt(unsigned cnt, ...)
{
    static thread_local struct curl_slist items[8] = {0};
    assert(cnt < ARRAY_SIZE(items));

    va_list valist;
    va_start(valist, cnt);

    for (unsigned i = 0; i + 1 < cnt; ++i)
    {
        items[i].data = va_arg(valist, char *);
        items[i].next = items + 1;
    }

    items[cnt - 1].data = va_arg(valist, char *);
    items[cnt - 1].next = NULL;

    va_end(valist);

    return items;
}
