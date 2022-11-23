#ifndef LOGY_H
#define LOGY_H

#include "lazylog.h"
#include "rc.h"
#include <stdlib.h>

#define debug(...) zlog_debug(__VA_ARGS__)
#define info(...) zlog_info(__VA_ARGS__)
#define warn(...) zlog_warn(__VA_ARGS__)
#define error(...) zlog_error(__VA_ARGS__)
#define fatal(...)                                                             \
    do                                                                         \
    {                                                                          \
        zlog_fatal(__VA_ARGS__);                                               \
        exit(1);                                                               \
    } while (0)

#define etimeout(...)                                                          \
    ({                                                                         \
        error(__VA_ARGS__);                                                    \
        RC_TIMEDOUT;                                                           \
    })

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

#define ehttp(...)                                                             \
    ({                                                                         \
        error(__VA_ARGS__);                                                    \
        RC_EHTTP;                                                              \
    })

#endif
