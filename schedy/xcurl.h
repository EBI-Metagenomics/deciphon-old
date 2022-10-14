#ifndef SCHEDY_XCURL_H
#define SCHEDY_XCURL_H

#include "core/rc.h"
#include <stdio.h>

struct mime_file;

enum rc xcurl_init(char const *url_stem, char const *api_key);
void xcurl_cleanup(void);
long xcurl_http_code(void);

size_t xcurl_body_size(void);
char *xcurl_body_data(void);

void xcurl_mime_init(void);
void xcurl_mime_addfile(char const *name, char const *filename,
                        char const *type, char const *filepath);
void xcurl_mime_addpart(char const *name, char const *data);
void xcurl_mime_cleanup(void);

enum rc xcurl_get(char const *query);
enum rc xcurl_post(char const *query, char const *json);
enum rc xcurl_patch(char const *query, char const *json);
enum rc xcurl_delete(char const *query);
enum rc xcurl_download(char const *query, FILE *fp);
enum rc xcurl_upload(char const *query);

#endif