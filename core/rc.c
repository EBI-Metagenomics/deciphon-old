#include "deciphon/rc.h"
#include "deciphon/compiler.h"
#include "deciphon/logger.h"
#include <string.h>

static char const name[][9] = {"ok",  "end",    "efail", "einval",
                               "eio", "enomem", "eparse"};

enum rc resolve_rc(unsigned len, char const *str, enum rc *rc)
{
    for (unsigned i = 0; i < ARRAY_SIZE(name); ++i)
    {
        if (!strncmp(name[i], str, len))
        {
            *rc = (enum rc)i;
            return RC_OK;
        }
    }
    return einval("invalid return code");
}
