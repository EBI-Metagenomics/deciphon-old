#ifndef CORE_XCURL_HEADER_H
#define CORE_XCURL_HEADER_H

#include "core/pp.h"

struct curl_slist;

struct curl_slist const *_xcurl_header(int cnt, ...);

#define xcurl_header(...) _xcurl_header(PP_NARG(__VA_ARGS__), __VA_ARGS__)

#endif
