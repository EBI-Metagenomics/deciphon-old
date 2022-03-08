#ifndef JSMN_ERROR_H
#define JSMN_ERROR_H

#include "deciphon/logger.h"
#include "jsmn.h"

/* clang-format off */
#define jsmn_error(code)                                                       \
    code == JSMN_ERROR_NOMEM                                                   \
        ? efail("Not enough tokens")                                           \
    : code == JSMN_ERROR_INVAL                                                 \
        ? efail("Invalid character")                                           \
    : code == JSMN_ERROR_PART                                                  \
        ? efail("Not a full JSON packet")                                      \
    : efail("Unknown error")
/* clang-format on */

#endif
