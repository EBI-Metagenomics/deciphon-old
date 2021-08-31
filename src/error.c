#include "error.h"
#include "log.h"

enum dcp_rc __error(enum dcp_rc rc, char const *msg)
{
    log_print(msg);
    return rc;
}
