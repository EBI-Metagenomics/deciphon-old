#ifndef ERROR_H
#define ERROR_H

#include "dcp/rc.h"
#include "macros.h"

#define __ERROR_FMT(rc, msg) __LOCAL(__LINE__) ":" #rc ": " msg

enum dcp_rc __dcp_error(enum dcp_rc rc, char const *msg);

#define error(rc, msg) __dcp_error(rc, __ERROR_FMT(rc, msg))

#endif
