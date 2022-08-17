#ifndef CORE_XCURL_MIME_H
#define CORE_XCURL_MIME_H

#include "deciphon/core/xcurl_mime.h"
#include <curl/curl.h>

curl_mime *xcurl_mime_new_file(CURL *curl, struct xcurl_mime_file const *,
                               char const *filepath);
void xcurl_mime_del(curl_mime *);

#endif
