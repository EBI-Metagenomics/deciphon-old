#ifndef SCHEDY_HTTP_HEADERS_H
#define SCHEDY_HTTP_HEADERS_H

#include "mime.h"

#define ACCEPT_JSON "Accept: " MIME_JSON
#define ACCEPT_PLAIN "Accept: " MIME_PLAIN
#define ACCEPT_TAB "Accept: " MIME_TAB
#define ACCEPT_OCTET "Accept: " MIME_OCTET

#define CONTENT_JSON "Content-Type: " MIME_JSON
#define CONTENT_PLAIN "Content-Type: " MIME_PLAIN
#define CONTENT_TAB "Content-Type: " MIME_TAB
#define CONTENT_OCTET "Content-Type: " MIME_OCTET

#endif
