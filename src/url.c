#include "url.h"
#include "array_size.h"
#include "zc.h"
#include <assert.h>

bool url_init(struct url *url, char const *stem)
{
    url->stem = url->full;
    unsigned n = (unsigned)zc_strlcpy(url->stem, stem, array_size(url->full));
    if (n >= URL_STEM_SIZE) return false;

    url->query = url->full + n;
    return true;
}

static inline unsigned __url_size_left(struct url const *url)
{
    return (unsigned)(&url->full[array_size(url->full)] - url->query);
}

void url_set_query(struct url *url, char const *query)
{
    unsigned sz = __url_size_left(url);
    unsigned n = (unsigned)zc_strlcpy(url->query, query, sz);
    (void)n;
    assert(n < URL_QUERY_SIZE);
}
