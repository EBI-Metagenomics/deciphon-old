#ifndef UTC_H
#define UTC_H

#include <stdint.h>
#include <time.h>

typedef uint64_t dcp_utc;

#define DCP_UTC_NULL 0

static inline dcp_utc dcp_utc_now(void) { return (dcp_utc)time(NULL); }

#endif
