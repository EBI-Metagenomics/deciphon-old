#include "xcurl.h"
#include "core/buff.h"
#include "core/compiler.h"
#include "core/limits.h"
#include "core/logy.h"
#include "core/pp.h"
#include "core/url.h"
#include "http_headers.h"
#include "mime.h"
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
    long http_code;
    struct buff *body;
    struct
    {
        curl_mime *form;
        curl_mimepart *field;
    } mime;
} x = {0};

#define http_header(...) http_header_cnt(PP_NARG(__VA_ARGS__), __VA_ARGS__)
static struct curl_slist const *http_header_cnt(unsigned cnt, ...);
static size_t noop_write(void *ptr, size_t size, size_t nmemb, void *data);
static size_t write_callback(void *data, size_t, size_t size, void *);
static enum rc perform_request(void);
static void setup_write_callback(void);

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
    x.curl = NULL;

    if (x.body) buff_del(x.body);
    x.body = NULL;

    curl_global_cleanup();
}

long xcurl_http_code(void) { return x.http_code; }

size_t xcurl_body_size(void) { return x.body->size; }

char *xcurl_body_data(void) { return x.body->data; }

void xcurl_mime_init(void)
{
    x.mime.form = curl_mime_init(x.curl);
    if (!x.mime.form) enomem("failed to allocate for mime");
}

void xcurl_mime_addfile(char const *name, char const *filename,
                        char const *type, char const *filepath)
{
    if (!(x.mime.field = curl_mime_addpart(x.mime.form))) efail("fail");
    if (curl_mime_name(x.mime.field, "name")) efail("fail");
    if (curl_mime_data(x.mime.field, name, CURL_ZERO_TERMINATED)) efail("fail");

    if (!(x.mime.field = curl_mime_addpart(x.mime.form))) efail("fail");
    if (curl_mime_name(x.mime.field, name)) efail("fail");
    if (curl_mime_type(x.mime.field, type)) efail("fail");
    if (curl_mime_filedata(x.mime.field, filepath)) efail("fail");

    if (!(x.mime.field = curl_mime_addpart(x.mime.form))) efail("fail");
    if (curl_mime_name(x.mime.field, "filename")) efail("fail");
    if (curl_mime_data(x.mime.field, filename, CURL_ZERO_TERMINATED))
        efail("fail");
}

void xcurl_mime_addpart(char const *name, char const *data)
{
    if (!(x.mime.field = curl_mime_addpart(x.mime.form))) efail("fail");
    if (curl_mime_name(x.mime.field, name)) efail("fail");
    if (curl_mime_data(x.mime.field, data, CURL_ZERO_TERMINATED)) efail("fail");
}

void xcurl_mime_cleanup(void) { curl_mime_free(x.mime.form); }

enum rc xcurl_get(char const *query)
{
    url_set_query(&x.url, query);
    curl_easy_setopt(x.curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(x.curl, CURLOPT_URL, x.url.full);
    curl_easy_setopt(x.curl, CURLOPT_CONNECTTIMEOUT, CONNECTTIMEOUT);
    setup_write_callback();

    curl_easy_setopt(x.curl, CURLOPT_HTTPHEADER,
                     http_header(x.x_api_key, ACCEPT_JSON));
    curl_easy_setopt(x.curl, CURLOPT_HTTPGET, 1L);

    return perform_request();
}

enum rc xcurl_post(char const *query, char const *json)
{
    url_set_query(&x.url, query);
    curl_easy_setopt(x.curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(x.curl, CURLOPT_URL, x.url.full);
    curl_easy_setopt(x.curl, CURLOPT_CONNECTTIMEOUT, CONNECTTIMEOUT);
    setup_write_callback();

    curl_easy_setopt(x.curl, CURLOPT_HTTPHEADER,
                     http_header(x.x_api_key, ACCEPT_JSON, CONTENT_JSON));
    curl_easy_setopt(x.curl, CURLOPT_POST, 1L);
    curl_easy_setopt(x.curl, CURLOPT_POSTFIELDS, json);

    xcurl_debug_setup(x.curl);

    return perform_request();
}

enum rc xcurl_patch(char const *query, char const *json)
{
    url_set_query(&x.url, query);
    curl_easy_setopt(x.curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(x.curl, CURLOPT_URL, x.url.full);
    curl_easy_setopt(x.curl, CURLOPT_CONNECTTIMEOUT, CONNECTTIMEOUT);
    setup_write_callback();

    curl_easy_setopt(x.curl, CURLOPT_HTTPHEADER,
                     http_header(x.x_api_key, ACCEPT_JSON, CONTENT_JSON));
    curl_easy_setopt(x.curl, CURLOPT_POSTFIELDS, json);
    curl_easy_setopt(x.curl, CURLOPT_CUSTOMREQUEST, "PATCH");

    xcurl_debug_setup(x.curl);

    return perform_request();
}

enum rc xcurl_delete(char const *query)
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

    return perform_request();
}

enum rc xcurl_download(char const *query, FILE *fp)
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

    return perform_request();
}

enum rc xcurl_upload(char const *query)
{
    url_set_query(&x.url, query);
    curl_easy_setopt(x.curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(x.curl, CURLOPT_URL, x.url.full);
    curl_easy_setopt(x.curl, CURLOPT_CONNECTTIMEOUT, CONNECTTIMEOUT);
    curl_easy_setopt(x.curl, CURLOPT_TIMEOUT, TIMEOUT);
    setup_write_callback();

    curl_easy_setopt(x.curl, CURLOPT_HTTPHEADER,
                     http_header(x.x_api_key, ACCEPT_JSON));

    curl_easy_setopt(x.curl, CURLOPT_MIMEPOST, x.mime.form);

    xcurl_debug_setup(x.curl);

    return perform_request();
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

static size_t noop_write(void *ptr, size_t size, size_t nmemb, void *data)
{
    (void)ptr;
    (void)data;
    return size * nmemb;
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

static enum rc perform_request(void)
{
    CURLcode code = curl_easy_perform(x.curl);
    if (code) return xcurl_error(code);
    curl_easy_getinfo(x.curl, CURLINFO_RESPONSE_CODE, &x.http_code);
    curl_easy_reset(x.curl);
    return RC_OK;
}

static void setup_write_callback(void)
{
    x.body->data[0] = '\0';
    x.body->size = 1;
    curl_easy_setopt(x.curl, CURLOPT_WRITEFUNCTION, write_callback);
}
