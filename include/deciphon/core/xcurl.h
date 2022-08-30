#ifndef DECIPHON_CORE_XCURL_H
#define DECIPHON_CORE_XCURL_H

#include "deciphon/core/limits.h"
#include "deciphon/core/rc.h"
#include "deciphon/core/url.h"
#include "sched/structs.h"
#include <stdio.h>

struct xcurl;
struct xcurl_mime_file;

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
                     struct xcurl_mime_file const *mime, char const *filepath);

enum rc xcurl_upload2(char const *query, long *http_code,
                      xcurl_callback_func_t callback, void *arg, int64_t db_id,
                      bool multi_hits, bool hmmer3_compat,
                      struct xcurl_mime_file const *mime, char const *filepath);

#endif
