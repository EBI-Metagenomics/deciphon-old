#ifndef PROTEIN_ID_H
#define PROTEIN_ID_H

#include "dcp_limits.h"

enum protein_state_id
{
    PROTEIN_ID_MATCH = (0 << (DCP_BITS_PER_PROFILE_TYPEID - 2)),
    PROTEIN_ID_INSERT = (1 << (DCP_BITS_PER_PROFILE_TYPEID - 2)),
    PROTEIN_ID_DELETE = (2 << (DCP_BITS_PER_PROFILE_TYPEID - 2)),
    PROTEIN_ID_EXT = (3 << (DCP_BITS_PER_PROFILE_TYPEID - 2)),
    PROTEIN_ID_R = (PROTEIN_ID_EXT | 0),
    PROTEIN_ID_S = (PROTEIN_ID_EXT | 1),
    PROTEIN_ID_N = (PROTEIN_ID_EXT | 2),
    PROTEIN_ID_B = (PROTEIN_ID_EXT | 3),
    PROTEIN_ID_E = (PROTEIN_ID_EXT | 4),
    PROTEIN_ID_J = (PROTEIN_ID_EXT | 5),
    PROTEIN_ID_C = (PROTEIN_ID_EXT | 6),
    PROTEIN_ID_T = (PROTEIN_ID_EXT | 7),
};

#endif
