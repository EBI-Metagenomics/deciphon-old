#ifndef CORE_MIME_H
#define CORE_MIME_H

#include <curl/curl.h>

struct mime_file
{
    char const *name;
    char const *filename;
    char const *type;
};

#define MIME_FILE_DEF(var, name, filename, type)                               \
    struct mime_file var = (struct mime_file){name, filename, type};

#define MIME_JSON "application/json"
#define MIME_PLAIN "text/plain"
#define MIME_TAB "text/tab-separated-values"
#define MIME_OCTET "application/octet-stream"

curl_mime *mime_new_file(CURL *curl, struct mime_file const *,
                         char const *filepath);
void mime_del(curl_mime *);

#endif
