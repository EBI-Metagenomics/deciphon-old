#include <assert.h>
#include <curl/curl.h>
#include <stdarg.h>

enum
{
    NODES_MAX_COUNT = 8
};

static struct curl_slist nodes[NODES_MAX_COUNT];

struct curl_slist const *_xcurl_header(int cnt, ...)
{
    if (cnt <= 0) return 0;
    assert(cnt < NODES_MAX_COUNT);

    va_list valist;
    va_start(valist, cnt);

    for (int i = 0; i < cnt - 1; ++i)
    {
        nodes[i].data = va_arg(valist, char *);
        nodes[i].next = nodes + 1;
    }

    nodes[cnt - 1].data = va_arg(valist, char *);
    nodes[cnt - 1].next = 0;

    va_end(valist);

    return nodes;
}
