#ifndef DECIPHON_CORE_LOGGING_H
#define DECIPHON_CORE_LOGGING_H

#include "deciphon/core/bug.h"
#include "deciphon/core/compiler.h"
#include "deciphon/core/rc.h"
#include <stdbool.h>
#include <stdio.h>

enum logging_level
{
    LOGGING_DEBUG,
    LOGGING_INFO,
    LOGGING_WARN,
    LOGGING_ERROR,
    LOGGING_FATAL
};

#define debug(msg) __logging_print(LOGGING_DEBUG, LOCAL, msg)
#define info(msg) __logging_print(LOGGING_INFO, LOCAL, msg)
#define warn(msg) __logging_print(LOGGING_WARN, LOCAL, msg)
#define error(msg) __logging_print(LOGGING_ERROR, LOCAL, msg)
#define fatal(msg) __logging_print(LOGGING_FATAL, LOCAL, msg)

void __logging_print(enum logging_level level, char const *ctx, char const *fmt,
                     ...);

void logging_setup(FILE *restrict user_stream, enum logging_level user_level,
                   FILE *restrict sys_stream, enum logging_level sys_level);

#define efail(msg)                                                             \
    ({                                                                         \
        error(msg);                                                            \
        RC_EFAIL;                                                              \
    })

#define einval(msg)                                                            \
    ({                                                                         \
        error(msg);                                                            \
        RC_EINVAL;                                                             \
    })

#define eio(msg)                                                               \
    ({                                                                         \
        error(msg);                                                            \
        RC_EIO;                                                                \
    })

#define enomem(msg)                                                            \
    ({                                                                         \
        error(msg);                                                            \
        RC_ENOMEM;                                                             \
    })

// RC_OK,
// RC_END,
// RC_EFAIL,
// RC_EINVAL,
// RC_EIO,
// RC_ENOMEM,
// RC_EPARSE,
// RC_EREST,
// RC_EHTTP,

#endif
