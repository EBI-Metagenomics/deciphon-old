#ifndef CORE_XCURL_H
#define CORE_XCURL_H

#include "deciphon/core/rc.h"
#include <stdbool.h>
#include <stdio.h>

struct mime_file;

enum rc xcurl_init(char const *url_stem, char const *api_key);
void xcurl_cleanup(void);

size_t xcurl_body_size(void);
char *xcurl_body_data(void);

enum rc xcurl_get(char const *query, long *http);
enum rc xcurl_post(char const *query, long *http, char const *json);
enum rc xcurl_patch(char const *query, long *http, char const *json);
enum rc xcurl_delete(char const *query, long *http);
enum rc xcurl_download(char const *query, long *http, FILE *fp);
enum rc xcurl_upload(char const *query, long *http,
                     struct mime_file const *mime, char const *filepath);
enum rc xcurl_upload2(char const *query, long *http, int64_t db_id,
                      bool multi_hits, bool hmmer3_compat,
                      struct mime_file const *mime, char const *filepath);

#endif
