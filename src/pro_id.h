#ifndef DCP_PRO_ID_H
#define DCP_PRO_ID_H

#include "prof_types.h"

#define DCP_PRO_ID_MATCH (0U << (DCP_PROFILE_BITS_ID - 2))
#define DCP_PRO_ID_INSERT (1U << (DCP_PROFILE_BITS_ID - 2))
#define DCP_PRO_ID_DELETE (2U << (DCP_PROFILE_BITS_ID - 2))
#define DCP_PRO_ID_EXT (3U << (DCP_PROFILE_BITS_ID - 2))
#define DCP_PRO_ID_R (DCP_PRO_ID_EXT | 0U)
#define DCP_PRO_ID_S (DCP_PRO_ID_EXT | 1U)
#define DCP_PRO_ID_N (DCP_PRO_ID_EXT | 2U)
#define DCP_PRO_ID_B (DCP_PRO_ID_EXT | 3U)
#define DCP_PRO_ID_E (DCP_PRO_ID_EXT | 4U)
#define DCP_PRO_ID_J (DCP_PRO_ID_EXT | 5U)
#define DCP_PRO_ID_C (DCP_PRO_ID_EXT | 6U)
#define DCP_PRO_ID_T (DCP_PRO_ID_EXT | 7U)

#endif
