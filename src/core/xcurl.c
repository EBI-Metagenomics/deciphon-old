#include "deciphon/core/xcurl.h"
#include "core/http_headers.h"
#include "core/mime.h"
#include "core/pp.h"
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
#define API_KEY_LENGTH 256

static struct xcurl
{
    CURL *curl;
    char x_api_key[API_KEY_LENGTH + 16];
    struct url url;
    struct buff *body;
} x = {0};

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
    x.body->data[0] = '\0';
    x.body->size = 1;
}

enum rc xcurl_init(char const *url_stem, char const *api_key)
{
    if (strlen(api_key) > API_KEY_LENGTH) return einval("api key is too long");
    if (!url_init(&x.url, url_stem)) return einval("invalid url stem");

    strcpy(x.x_api_key, "X-API-KEY: ");
    strcat(x.x_api_key, api_key);

    enum rc rc = RC_OK;

    if (curl_global_init(CURL_GLOBAL_DEFAULT))
    {
        rc = efail("failed to initialized curl");
        goto cleanup;
    }

    if (!(x.body = buff_new(1024)))
    {
        rc = enomem("failed to aloccate for body");
        goto cleanup;
    }

    if (!(x.curl = curl_easy_init()))
    {
        rc = efail("failed to initialize curl");
        goto cleanup;
    }

    return rc;

cleanup:
    xcurl_cleanup();
    return rc;
}

void xcurl_cleanup(void)
{
    if (x.curl) curl_easy_cleanup(x.curl);
    x.curl = 0;

    if (x.body) buff_del(x.body);
    x.body = 0;

    curl_global_cleanup();
}

static size_t write_callback(void *data, size_t one, size_t size, void *arg)
{
    (void)one;
    (void)arg;

    if (!buff_ensure(&x.body, x.body->size + size))
    {
        enomem("buff_ensure failed");
        return 0;
    }

    memcpy(x.body->data + x.body->size - 1, data, size);
    x.body->size += size;
    x.body->data[x.body->size - 1] = '\0';

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

size_t xcurl_body_size(void) { return x.body->size; }

char *xcurl_body_data(void) { return x.body->data; }

enum rc xcurl_get(char const *query, long *http)
{
    url_set_query(&x.url, query);
    curl_easy_setopt(x.curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(x.curl, CURLOPT_URL, x.url.full);
    curl_easy_setopt(x.curl, CURLOPT_CONNECTTIMEOUT, CONNECTTIMEOUT);

    body_reset();
    curl_easy_setopt(x.curl, CURLOPT_WRITEFUNCTION, write_callback);

    curl_easy_setopt(x.curl, CURLOPT_HTTPHEADER,
                     http_header(x.x_api_key, ACCEPT_JSON));
    curl_easy_setopt(x.curl, CURLOPT_HTTPGET, 1L);

    return perform_request(x.curl, http);
}

enum rc xcurl_post(char const *query, long *http_code, char const *json)
{
    url_set_query(&x.url, query);
    curl_easy_setopt(x.curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(x.curl, CURLOPT_URL, x.url.full);
    curl_easy_setopt(x.curl, CURLOPT_CONNECTTIMEOUT, CONNECTTIMEOUT);

    body_reset();
    curl_easy_setopt(x.curl, CURLOPT_WRITEFUNCTION, write_callback);

    curl_easy_setopt(x.curl, CURLOPT_HTTPHEADER,
                     http_header(x.x_api_key, ACCEPT_JSON, CONTENT_JSON));
    curl_easy_setopt(x.curl, CURLOPT_POST, 1L);
    curl_easy_setopt(x.curl, CURLOPT_POSTFIELDS, json);

    xcurl_debug_setup(x.curl);

    return perform_request(x.curl, http_code);
}

enum rc xcurl_patch(char const *query, long *http_code, char const *json)
{
    url_set_query(&x.url, query);
    curl_easy_setopt(x.curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(x.curl, CURLOPT_URL, x.url.full);
    curl_easy_setopt(x.curl, CURLOPT_CONNECTTIMEOUT, CONNECTTIMEOUT);

    body_reset();
    curl_easy_setopt(x.curl, CURLOPT_WRITEFUNCTION, write_callback);

    curl_easy_setopt(x.curl, CURLOPT_HTTPHEADER,
                     http_header(x.x_api_key, ACCEPT_JSON, CONTENT_JSON));
    curl_easy_setopt(x.curl, CURLOPT_POSTFIELDS, json);
    curl_easy_setopt(x.curl, CURLOPT_CUSTOMREQUEST, "PATCH");

    xcurl_debug_setup(x.curl);

    return perform_request(x.curl, http_code);
}

enum rc xcurl_delete(char const *query, long *http_code)
{
    url_set_query(&x.url, query);
    curl_easy_setopt(x.curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(x.curl, CURLOPT_URL, x.url.full);
    curl_easy_setopt(x.curl, CURLOPT_CONNECTTIMEOUT, CONNECTTIMEOUT);

    curl_easy_setopt(x.curl, CURLOPT_HTTPHEADER,
                     http_header(x.x_api_key, ACCEPT_JSON));
    curl_easy_setopt(x.curl, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(x.curl, CURLOPT_WRITEFUNCTION, noop_write);
    curl_easy_setopt(x.curl, CURLOPT_CUSTOMREQUEST, "DELETE");

    xcurl_debug_setup(x.curl);

    return perform_request(x.curl, http_code);
}

enum rc xcurl_download(char const *query, long *http_code, FILE *fp)
{
    url_set_query(&x.url, query);
    curl_easy_setopt(x.curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(x.curl, CURLOPT_URL, x.url.full);
    curl_easy_setopt(x.curl, CURLOPT_CONNECTTIMEOUT, CONNECTTIMEOUT);
    curl_easy_setopt(x.curl, CURLOPT_TIMEOUT, TIMEOUT);

    curl_easy_setopt(x.curl, CURLOPT_WRITEFUNCTION, 0);
    curl_easy_setopt(x.curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(x.curl, CURLOPT_HTTPGET, 1L);

    xcurl_debug_setup(x.curl);

    return perform_request(x.curl, http_code);
}

enum rc xcurl_upload(char const *query, long *http_code,
                     struct mime_file const *mime, char const *filepath)
{
    url_set_query(&x.url, query);
    curl_easy_setopt(x.curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(x.curl, CURLOPT_URL, x.url.full);
    curl_easy_setopt(x.curl, CURLOPT_CONNECTTIMEOUT, CONNECTTIMEOUT);
    curl_easy_setopt(x.curl, CURLOPT_TIMEOUT, TIMEOUT);

    body_reset();
    curl_easy_setopt(x.curl, CURLOPT_WRITEFUNCTION, write_callback);

    curl_easy_setopt(x.curl, CURLOPT_HTTPHEADER,
                     http_header(x.x_api_key, ACCEPT_JSON));

    curl_mime *form = mime_new_file(x.curl, mime, filepath);
    if (!form) return enomem("failed to allocate for mime");
    curl_easy_setopt(x.curl, CURLOPT_MIMEPOST, form);

    xcurl_debug_setup(x.curl);

    enum rc rc = perform_request(x.curl, http_code);
    mime_del(form);

    return rc;
}

enum rc xcurl_upload2(char const *query, long *http_code, int64_t db_id,
                      bool multi_hits, bool hmmer3_compat,
                      struct mime_file const *mime, char const *filepath)
{
    url_set_query(&x.url, query);
    curl_easy_setopt(x.curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(x.curl, CURLOPT_URL, x.url.full);
    curl_easy_setopt(x.curl, CURLOPT_CONNECTTIMEOUT, CONNECTTIMEOUT);
    curl_easy_setopt(x.curl, CURLOPT_TIMEOUT, TIMEOUT);

    body_reset();
    curl_easy_setopt(x.curl, CURLOPT_WRITEFUNCTION, write_callback);

    curl_easy_setopt(x.curl, CURLOPT_HTTPHEADER,
                     http_header(x.x_api_key, ACCEPT_JSON));

    curl_mime *form = curl_mime_init(x.curl);

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
    curl_easy_setopt(x.curl, CURLOPT_MIMEPOST, form);

    xcurl_debug_setup(x.curl);

    enum rc rc = perform_request(x.curl, http_code);
    mime_del(form);

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
