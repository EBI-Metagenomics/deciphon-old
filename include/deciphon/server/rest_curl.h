#ifndef DECIPHON_SERVER_REST_CURL_H
#define DECIPHON_SERVER_REST_CURL_H

#include "deciphon/rc.h"
#include "deciphon/server/rest_url.h"
#include "deciphon/spinlock.h"
#include <curl/curl.h>
#include <stdatomic.h>
#include <stdbool.h>

struct rest_curl
{
    CURL *handle;
    spinlock_t lock;
    struct
    {
        struct curl_slist *get;
        struct curl_slist *post;
    } headers;
    struct rest_url url;
};

enum rc rest_curl_init(struct rest_curl *curl, char const *url_stem);
void rest_curl_del(struct rest_curl *curl);

typedef size_t (*rest_curl_callback_func_t)(void *contents, size_t size,
                                            size_t nmemb, void *userp);

enum rc rest_curl_http_get(struct rest_curl *curl, char const *query,
                           rest_curl_callback_func_t callback, void *arg);
enum rc rest_curl_http_post(struct rest_curl *curl, char const *query,
                            char const *json);

#endif
