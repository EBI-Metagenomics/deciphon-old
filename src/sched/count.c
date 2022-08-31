#include "sched/count.h"
#include "deciphon/core/logging.h"
#include "xjson.h"

enum rc count_parse(struct count *count, struct xjson *x, unsigned start)
{
    enum rc rc = RC_OK;
    static unsigned expected_items = 1;
    int64_t count64 = 0;

    unsigned nitems = 0;
    for (unsigned i = start; i < x->ntoks && nitems < expected_items; i += 2)
    {
        if (xjson_eqstr(x, i, "count"))
        {
            if ((rc = xjson_bind_int64(x, i + 1, &count64))) return rc;
        }
        else
            return einval("unexpected json key");
        nitems++;
    }

    if (nitems != expected_items) return einval("expected four items");

    count->count = count64;

    return RC_OK;
}
