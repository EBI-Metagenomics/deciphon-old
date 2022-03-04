#ifndef DECIPHON_REST_URL_H
#define DECIPHON_REST_URL_H

#include "deciphon/compiler.h"
#include "deciphon/util/util.h"
#include <assert.h>
#include <stdbool.h>

enum
{
    REST_URL_STEM_SIZE = 512,
    REST_URL_QUERY_SIZE = 512,
};

struct rest_url
{
    char full[REST_URL_STEM_SIZE + REST_URL_QUERY_SIZE];
    char *stem;
    char *query;
};

static inline bool rest_url_init(struct rest_url *url, char const *stem)
{
    url->stem = url->full;
    unsigned n = (unsigned)strlcpy(url->stem, stem, ARRAY_SIZE(url->full));
    if (n >= REST_URL_STEM_SIZE) return false;

    url->query = url->full + n;
    return true;
}

static inline unsigned rest_url_query_size_left(struct rest_url const *url)
{
    return (unsigned)(&url->full[ARRAY_SIZE(url->full)] - url->query);
}

static inline void rest_url_set_query(struct rest_url *url, char const *query)
{
    unsigned sz = rest_url_query_size_left(url);
    unsigned n = (unsigned)strlcpy(url->query, query, sz);
    assert(n < REST_URL_QUERY_SIZE);
}

#if 0
#define rest_url_set_query(url, query)                                         \
    do                                                                         \
    {                                                                          \
        static_assert(ARRAY_SIZE(query) <= REST_URL_QUERY_SIZE, "too-long");   \
        __rest_url_set_query(url, STRADDR(query));                             \
    } while (0)
#endif

#endif
