#ifndef DECIPHON_CORE_LOGGING_H
#define DECIPHON_CORE_LOGGING_H

#include "deciphon/core/bug.h"
#include "deciphon/core/compiler.h"
#include "deciphon/core/rc.h"
#include <stdbool.h>
#include <stdio.h>

enum logging_level
{
    LOGGING_NOTSET,
    LOGGING_DEBUG,
    LOGGING_INFO,
    LOGGING_WARN,
    LOGGING_ERROR,
    LOGGING_FATAL
};

#define debug(...) __logging_print(LOGGING_DEBUG, LOCAL, __VA_ARGS__)
#define info(...) __logging_print(LOGGING_INFO, LOCAL, __VA_ARGS__)
#define warn(...) __logging_print(LOGGING_WARN, LOCAL, __VA_ARGS__)
#define error(...) __logging_print(LOGGING_ERROR, LOCAL, __VA_ARGS__)
#define fatal(...) __logging_print(LOGGING_FATAL, LOCAL, __VA_ARGS__)

void __logging_print(enum logging_level level, char const *ctx, char const *fmt,
                     ...);

void logging_setup(FILE *restrict user_stream, enum logging_level user_level,
                   FILE *restrict sys_stream, enum logging_level sys_level);

#define efail(...)                                                             \
    ({                                                                         \
        error(__VA_ARGS__);                                                    \
        RC_EFAIL;                                                              \
    })

#define einval(...)                                                            \
    ({                                                                         \
        error(__VA_ARGS__);                                                    \
        RC_EINVAL;                                                             \
    })

#define eio(...)                                                               \
    ({                                                                         \
        error(__VA_ARGS__);                                                    \
        RC_EIO;                                                                \
    })

#define enomem(...)                                                            \
    ({                                                                         \
        error(__VA_ARGS__);                                                    \
        RC_ENOMEM;                                                             \
    })

#define eparse(...)                                                            \
    ({                                                                         \
        error(__VA_ARGS__);                                                    \
        RC_EPARSE;                                                             \
    })

#define eapi(api_rc)                                                           \
    ({                                                                         \
        error(" api_rc[%d] %s", (api_rc).rc, (api_rc).msg);                    \
        RC_EAPI;                                                               \
    })

#define ehttp(...)                                                             \
    ({                                                                         \
        error(__VA_ARGS__);                                                    \
        RC_EHTTP;                                                              \
    })

#endif
