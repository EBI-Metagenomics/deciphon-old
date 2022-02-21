#include "common/rc.h"
#include "common/logger.h"
#include <string.h>

static char const name[9][9] = {"done",   "end", "next",   "notfound", "efail",
                                "einval", "eio", "enomem", "eparse"};

enum rc rc_from_str(unsigned len, char const *str, enum rc *rc)
{
    for (unsigned i = 0; i < 9; ++i)
    {
        if (!strncmp(name[i], str, len))
        {
            *rc = (enum rc)i;
            return DCP_OK;
        }
    }
    return error(DCP_EINVAL, "invalid return code");
}
