#include "deciphon/core/xcurl.h"
#include "core/http_headers.h"
#include "core/pp.h"
#include "core/xcurl_mime.h"
#include "deciphon/core/buff.h"
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
    struct url url;
    struct buff *body;
} xcurl = {0};

#define http_header(...) http_header_cnt(PP_NARG(__VA_ARGS__), __VA_ARGS__)
static struct curl_slist const *http_header_cnt(unsigned cnt, ...);

static size_t noop_write(void *ptr, size_t size, size_t nmemb, void *data)
{
    (void)ptr;
    (void)data;
    return size * nmemb;
}

static inline void body_reset(void)
{
    xcurl.body->data[0] = '\0';
    xcurl.body->size = 1;
}

enum rc xcurl_init(char const *url_stem, char const *api_key)
{
    enum rc rc = RC_OK;

    if (curl_global_init(CURL_GLOBAL_DEFAULT))
    {
        rc = efail("failed to initialized curl");
        goto cleanup;
    }

    if (!(xcurl.body = buff_new(1024)))
    {
        rc = enomem("failed to aloccate for body");
        goto cleanup;
    }
    body_reset();

    if (!(xcurl.curl = curl_easy_init()))
    {
        rc = efail("failed to initialize curl");
        goto cleanup;
    }

    if (!url_init(&xcurl.url, url_stem))
    {
        rc = einval("invalid url stem");
        goto cleanup;
    }

    strcpy(api_key_header + strlen(api_key_header), api_key);
    return rc;

cleanup:
    xcurl_cleanup();
    return rc;
}

void xcurl_cleanup(void)
{
    if (xcurl.curl) curl_easy_cleanup(xcurl.curl);
    xcurl.curl = 0;

    if (xcurl.body) buff_del(xcurl.body);
    xcurl.body = 0;

    curl_global_cleanup();
}

// struct callback_data
// {
//     xcurl_cb_t *callback;
//     void *arg;
// };

static size_t write_callback(void *data, size_t one, size_t size, void *arg)
{
    (void)one;
    (void)arg;

    if (!buff_ensure(&xcurl.body, xcurl.body->size + size))
    {
        enomem("buff_ensure failed");
        return 0;
    }

    memcpy(xcurl.body->data + xcurl.body->size - 1, data, size);
    xcurl.body->size += size;
    xcurl.body->data[xcurl.body->size - 1] = '\0';

    return size;
}

static enum rc perform_request(CURL *curl, long *http_code)
{
    CURLcode code = curl_easy_perform(curl);
    if (code) return xcurl_error(code);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, http_code);

    curl_easy_reset(curl);
    return RC_OK;
}

size_t xcurl_body_size(void) { return xcurl.body->size; }

char *xcurl_body_data(void) { return xcurl.body->data; }

enum rc xcurl_get(char const *query, long *http)
{
    url_set_query(&xcurl.url, query);
    curl_easy_setopt(xcurl.curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(xcurl.curl, CURLOPT_URL, xcurl.url.full);
    curl_easy_setopt(xcurl.curl, CURLOPT_CONNECTTIMEOUT, CONNECTTIMEOUT);

    body_reset();
    curl_easy_setopt(xcurl.curl, CURLOPT_WRITEFUNCTION, write_callback);
    // struct callback_data cd = {callback, arg};
    // curl_easy_setopt(xcurl.curl, CURLOPT_WRITEDATA, &cd);

    curl_easy_setopt(xcurl.curl, CURLOPT_HTTPHEADER,
                     http_header(api_key_header, ACCEPT_JSON));
    curl_easy_setopt(xcurl.curl, CURLOPT_HTTPGET, 1L);

    return perform_request(xcurl.curl, http);
}

enum rc xcurl_post(char const *query, long *http_code, char const *json)
{
    url_set_query(&xcurl.url, query);
    curl_easy_setopt(xcurl.curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(xcurl.curl, CURLOPT_URL, xcurl.url.full);
    curl_easy_setopt(xcurl.curl, CURLOPT_CONNECTTIMEOUT, CONNECTTIMEOUT);

    body_reset();
    curl_easy_setopt(xcurl.curl, CURLOPT_WRITEFUNCTION, write_callback);
    // struct callback_data cd = {callback, arg};
    // curl_easy_setopt(xcurl.curl, CURLOPT_WRITEDATA, &cd);

    curl_easy_setopt(xcurl.curl, CURLOPT_HTTPHEADER,
                     http_header(api_key_header, ACCEPT_JSON, CONTENT_JSON));
    curl_easy_setopt(xcurl.curl, CURLOPT_POST, 1L);
    curl_easy_setopt(xcurl.curl, CURLOPT_POSTFIELDS, json);

    xcurl_debug_setup(xcurl.curl);

    return perform_request(xcurl.curl, http_code);
}

enum rc xcurl_patch(char const *query, long *http_code, char const *json)
{
    url_set_query(&xcurl.url, query);
    curl_easy_setopt(xcurl.curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(xcurl.curl, CURLOPT_URL, xcurl.url.full);
    curl_easy_setopt(xcurl.curl, CURLOPT_CONNECTTIMEOUT, CONNECTTIMEOUT);

    body_reset();
    curl_easy_setopt(xcurl.curl, CURLOPT_WRITEFUNCTION, write_callback);
    // struct callback_data cd = {callback, arg};
    // curl_easy_setopt(xcurl.curl, CURLOPT_WRITEDATA, &cd);

    curl_easy_setopt(xcurl.curl, CURLOPT_HTTPHEADER,
                     http_header(api_key_header, ACCEPT_JSON, CONTENT_JSON));
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

    curl_easy_setopt(xcurl.curl, CURLOPT_HTTPHEADER,
                     http_header(api_key_header, ACCEPT_JSON));
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
                     struct xcurl_mime_file const *mime, char const *filepath)
{
    url_set_query(&xcurl.url, query);
    curl_easy_setopt(xcurl.curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(xcurl.curl, CURLOPT_URL, xcurl.url.full);
    curl_easy_setopt(xcurl.curl, CURLOPT_CONNECTTIMEOUT, CONNECTTIMEOUT);
    curl_easy_setopt(xcurl.curl, CURLOPT_TIMEOUT, TIMEOUT);

    body_reset();
    curl_easy_setopt(xcurl.curl, CURLOPT_WRITEFUNCTION, write_callback);
    // struct callback_data cd = {callback, arg};
    // curl_easy_setopt(xcurl.curl, CURLOPT_WRITEDATA, &cd);

    curl_easy_setopt(xcurl.curl, CURLOPT_HTTPHEADER,
                     http_header(api_key_header, ACCEPT_JSON));

    curl_mime *form = xcurl_mime_new_file(xcurl.curl, mime, filepath);
    if (!form) return enomem("failed to allocate for mime");
    curl_easy_setopt(xcurl.curl, CURLOPT_MIMEPOST, form);

    xcurl_debug_setup(xcurl.curl);

    enum rc rc = perform_request(xcurl.curl, http_code);
    xcurl_mime_del(form);

    return rc;
}

enum rc xcurl_upload2(char const *query, long *http_code, int64_t db_id,
                      bool multi_hits, bool hmmer3_compat,
                      struct xcurl_mime_file const *mime, char const *filepath)
{
    url_set_query(&xcurl.url, query);
    curl_easy_setopt(xcurl.curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(xcurl.curl, CURLOPT_URL, xcurl.url.full);
    curl_easy_setopt(xcurl.curl, CURLOPT_CONNECTTIMEOUT, CONNECTTIMEOUT);
    curl_easy_setopt(xcurl.curl, CURLOPT_TIMEOUT, TIMEOUT);

    body_reset();
    curl_easy_setopt(xcurl.curl, CURLOPT_WRITEFUNCTION, write_callback);
    // struct callback_data cd = {callback, arg};
    // curl_easy_setopt(xcurl.curl, CURLOPT_WRITEDATA, &cd);

    curl_easy_setopt(xcurl.curl, CURLOPT_HTTPHEADER,
                     http_header(api_key_header, ACCEPT_JSON));

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
    static struct curl_slist items[8] = {0};
    assert(cnt < ARRAY_SIZE(items));

    va_list valist;
    va_start(valist, cnt);

    struct curl_slist *it = items;
    for (unsigned i = 0; i + 1 < cnt; ++i)
    {
        it->data = va_arg(valist, char *);
        it->next = it + 1;
        ++it;
    }

    it->data = va_arg(valist, char *);
    it->next = NULL;

    va_end(valist);

    return items;
}
