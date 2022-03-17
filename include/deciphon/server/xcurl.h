#ifndef DECIPHON_SERVER_XCURL_H
#define DECIPHON_SERVER_XCURL_H

#include "deciphon/limits.h"
#include "deciphon/rc.h"
#include "deciphon/server/url.h"
#include "deciphon/spinlock.h"
#include <curl/curl.h>
#include <stdatomic.h>
#include <stdbool.h>

struct xcurl_mime
{
    char name[FILENAME_SIZE];
    char filename[FILENAME_SIZE];
    char type[MIME_TYPE_SIZE];
};

void xcurl_mime_set(struct xcurl_mime *mime, char const *name,
                    char const *filename, char const *type);

struct xcurl
{
    CURL *curl;
    struct
    {
        struct curl_slist *send_json;
        struct curl_slist *recv_json;
        struct curl_slist *only_json;
    } hdr;
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

enum rc xcurl_upload(struct xcurl *x, char const *query, long *http_code,
                     struct xcurl_mime const *mime, char const *filepath);

#endif
