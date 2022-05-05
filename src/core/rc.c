#include "deciphon/core/rc.h"
#include "deciphon/core/compiler.h"
#include "deciphon/core/logging.h"
#include <string.h>

static char const name[][9] = {"ok",     "end",    "efail", "einval", "eio",
                               "enomem", "eparse", "erest", "ehttp"};

enum rc rc_resolve(unsigned len, char const *str, enum rc *rc)
{
    for (unsigned i = 0; i < ARRAY_SIZE(name); ++i)
    {
        if (!strncmp(name[i], str, len))
        {
            *rc = (enum rc)i;
            return RC_OK;
        }
    }
    warn("invalid return code");
    return RC_EINVAL;
}
