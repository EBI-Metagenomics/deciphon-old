#ifndef CORE_XCURL_HEADER_H
#define CORE_XCURL_HEADER_H

struct curl_slist;

struct curl_slist const *_xcurl_header(int cnt, ...);

#define NUMARGS(...) (sizeof((int[]){__VA_ARGS__}) / sizeof(int))
#define xcurl_header(...) _xcurl_header(NUMARGS(__VA_ARGS__), __VA_ARGS__)

#endif
