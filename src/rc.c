#include "rc.h"

static char const *strings[] = {
#define X(A, B) [A] = B,
    RC_MAP(X)
#undef X
};

char const *rc_string(int rc)
{
    if (rc < 0 || rc >= RC_ENUM_SIZE) return "unrecognized return code";
    return strings[rc];
}
