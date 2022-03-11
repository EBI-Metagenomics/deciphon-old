#ifndef DECIPHON_SERVER_XCURL_H
#define DECIPHON_SERVER_XCURL_H

#include "deciphon/rc.h"
#include "deciphon/server/url.h"
#include "deciphon/spinlock.h"
#include <curl/curl.h>
#include <stdatomic.h>
#include <stdbool.h>

struct xcurl
{
    CURL *curl;
    struct curl_slist *headers;
    struct url url;
};

enum rc xcurl_init(struct xcurl *, char const *url_stem);
void xcurl_del(struct xcurl *xcurl);

typedef size_t (*xcurl_callback_func_t)(void *data, size_t size, void *arg);

enum rc xcurl_get(struct xcurl *, char const *query, long *http_code,
                  xcurl_callback_func_t callback, void *arg);

enum rc xcurl_post(struct xcurl *, char const *query, long *http_code,
                   xcurl_callback_func_t callback, void *arg, char const *json);

enum rc xcurl_patch(struct xcurl *x, char const *query, long *http_code,
                    xcurl_callback_func_t callback, void *arg,
                    char const *json);

enum rc xcurl_delete(struct xcurl *, char const *query, long *http_code);

enum rc xcurl_download(struct xcurl *x, char const *query, long *http_code,
                       FILE *fp);

#endif
