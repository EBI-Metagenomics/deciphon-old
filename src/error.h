#ifndef ERROR_H
#define ERROR_H

#include "dcp/rc.h"

#define __ERROR_STRINGIFY(n) #n
#define __ERROR_LOCAL(n) __FILE__ ":" __ERROR_STRINGIFY(n)
#define __ERROR_FMT(rc, msg) __ERROR_LOCAL(__LINE__) ":" #rc ": " msg

enum dcp_rc __dcp_error(enum dcp_rc rc, char const *msg);

#define error(rc, msg) __dcp_error(rc, __ERROR_FMT(rc, msg))

#endif
