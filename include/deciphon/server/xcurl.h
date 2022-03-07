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
    CURL *handle;
    spinlock_t lock;
    struct
    {
        struct curl_slist *get;
        struct curl_slist *post;
    } headers;
    struct url url;
};

enum rc xcurl_init(struct xcurl *curl, char const *url_stem);
void xcurl_del(struct xcurl *curl);

typedef size_t (*xcurl_callback_func_t)(void *contents, size_t size,
                                        size_t nmemb, void *userp);

enum rc xcurl_http_get(struct xcurl *curl, char const *query, long *http_code,
                       xcurl_callback_func_t callback, void *arg);

enum rc xcurl_http_post(struct xcurl *curl, char const *query, long *http_code,
                        char const *json);

enum rc xcurl_http_delete(struct xcurl *curl, char const *query,
                          long *http_code);

#endif
