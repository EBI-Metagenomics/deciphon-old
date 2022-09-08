#include "sched/count.h"
#include "deciphon/core/logging.h"
#include "jx.h"

enum rc count_parse(struct count *count, struct jr *jr)
{
    count->count = (unsigned)jr_ulong_of(jr, "count");
    return jr_error() ? einval("failed to parse count") : RC_OK;
}
