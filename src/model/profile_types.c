#include "model/profile_typeid.h"

static char const *const names[] = {[PROFILE_NULL] = "null",
                                    [PROFILE_STANDARD] = "standard",
                                    [PROFILE_PROTEIN] = "protein"};

char const *profile_typeid_name(enum profile_typeid typeid)
{
    return names[typeid];
}
