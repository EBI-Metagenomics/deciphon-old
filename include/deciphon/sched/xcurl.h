#ifndef DECIPHON_SERVER_XCURL_H
#define DECIPHON_SERVER_XCURL_H

#include "deciphon/limits.h"
#include "deciphon/rc.h"
#include "deciphon/sched/url.h"
#include <stdio.h>

struct xcurl_mime
{
    char name[FILENAME_SIZE];
    char filename[FILENAME_SIZE];
    char type[MIME_TYPE_SIZE];
};

void xcurl_mime_set(struct xcurl_mime *, char const *name, char const *filename,
                    char const *type);

struct xcurl;

enum rc xcurl_init(char const *url_stem, char const *api_key);
void xcurl_cleanup(void);

typedef size_t (*xcurl_callback_func_t)(void *data, size_t size, void *arg);

enum rc xcurl_get(char const *query, long *http_code,
                  xcurl_callback_func_t callback, void *arg);

enum rc xcurl_post(char const *query, long *http_code,
                   xcurl_callback_func_t callback, void *arg, char const *json);

enum rc xcurl_patch(char const *query, long *http_code,
                    xcurl_callback_func_t callback, void *arg,
                    char const *json);

enum rc xcurl_delete(char const *query, long *http_code);

enum rc xcurl_download(char const *query, long *http_code, FILE *fp);

enum rc xcurl_upload(char const *query, long *http_code,
                     xcurl_callback_func_t callback, void *arg,
                     struct xcurl_mime const *mime, char const *filepath);

#endif
