#include "deciphon/sched/url.h"
#include "deciphon/strlcpy.h"

bool url_init(struct url *url, char const *stem)
{
    url->stem = url->full;
    unsigned n = (unsigned)dcp_strlcpy(url->stem, stem, ARRAY_SIZE(url->full));
    if (n >= URL_STEM_SIZE) return false;

    url->query = url->full + n;
    return true;
}

static inline unsigned __url_size_left(struct url const *url)
{
    return (unsigned)(&url->full[ARRAY_SIZE(url->full)] - url->query);
}

void url_set_query(struct url *url, char const *query)
{
    unsigned sz = __url_size_left(url);
    unsigned n = (unsigned)dcp_strlcpy(url->query, query, sz);
    assert(n < URL_QUERY_SIZE);
}
