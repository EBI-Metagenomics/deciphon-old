#include "deciphon/sched/url.h"
#include "c_toolbelt/c_toolbelt.h"

bool url_init(struct url *url, char const *stem)
{
    url->stem = url->full;
    unsigned n = (unsigned)ctb_strlcpy(url->stem, stem, ARRAY_SIZE(url->full));
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
    unsigned n = (unsigned)ctb_strlcpy(url->query, query, sz);
    (void)n;
    assert(n < URL_QUERY_SIZE);
}
