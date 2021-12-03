#ifndef PROTEIN_ID_H
#define PROTEIN_ID_H

#include "profile_types.h"

#define PROTEIN_ID_MATCH (0U << (DCP_PROFILE_BITS_ID - 2))
#define PROTEIN_ID_INSERT (1U << (DCP_PROFILE_BITS_ID - 2))
#define PROTEIN_ID_DELETE (2U << (DCP_PROFILE_BITS_ID - 2))
#define PROTEIN_ID_EXT (3U << (DCP_PROFILE_BITS_ID - 2))
#define PROTEIN_ID_R (PROTEIN_ID_EXT | 0U)
#define PROTEIN_ID_S (PROTEIN_ID_EXT | 1U)
#define PROTEIN_ID_N (PROTEIN_ID_EXT | 2U)
#define PROTEIN_ID_B (PROTEIN_ID_EXT | 3U)
#define PROTEIN_ID_E (PROTEIN_ID_EXT | 4U)
#define PROTEIN_ID_J (PROTEIN_ID_EXT | 5U)
#define PROTEIN_ID_C (PROTEIN_ID_EXT | 6U)
#define PROTEIN_ID_T (PROTEIN_ID_EXT | 7U)

#endif
