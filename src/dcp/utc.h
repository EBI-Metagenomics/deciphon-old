#ifndef DCP_UTC_H
#define DCP_UTC_H

#include "dcp/export.h"
#include <stdint.h>
#include <time.h>

typedef uint64_t dcp_utc;

static inline dcp_utc dcp_utc_now(void)
{
    return (dcp_utc)time(NULL);
}

#endif
