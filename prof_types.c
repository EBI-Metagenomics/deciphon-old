#include "prof_typeid.h"

static char const *const names[] = {
    [PROF_NULL] = "null", [PROF_STD] = "standard", [PROF_PROT] = "protein"};

char const *prof_typeid_name(enum prof_typeid typeid) { return names[typeid]; }
