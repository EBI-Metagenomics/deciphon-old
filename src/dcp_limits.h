#ifndef DCP_LIMITS_H
#define DCP_LIMITS_H

#include "compiler.h"
#include "imm/imm.h"

enum dcp_limits
{
    DCP_ABC_NAME_SIZE = 16,
    DCP_DB_NAME_SIZE = 64,
    DCP_PROFILE_TYPEID_SIZE = 16,
    DCP_FILENAME_SIZE = 128,
    DCP_PATH_SIZE = 4096,
    DCP_PROF_NAME_SIZE = 256,
    DCP_SEQ_NAME_SIZE = 256,
    DCP_SEQ_SIZE = (1024 * 1024),
    DCP_VERSION_SIZE = 16,
    DCP_MAX_NPROFILES = (1 << 20),
    DCP_PROFILE_NAME_SIZE = 64,
    DCP_PROFILE_ACC_SIZE = 32,
    DCP_MAX_OPEN_DB_FILES = 64,
    DCP_BITS_PER_PROFILE_ID = BITS_PER(imm_state_id_t),
};

#endif
