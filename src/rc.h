#ifndef RC_H
#define RC_H

#define RC_MAP(X)                                                              \
    X(RC_OK, "ok")                                                             \
    X(RC_END, "end")                                                           \
    X(RC_TIMEDOUT, "timed out")                                                \
    X(RC_EFAIL, "unspecified error")                                           \
    X(RC_EINVAL, "invalid values")                                             \
    X(RC_EIO, "i/o failure")                                                   \
    X(RC_ENOMEM, "not-enough-memory")                                          \
    X(RC_EPARSE, "parsing error")                                              \
    X(RC_EAPI, "api error")                                                    \
    X(RC_EHTTP, "http error")

enum rc
{
#define X(A, _) A,
    RC_MAP(X)
#undef X
        RC_ENUM_SIZE,
};

char const *rc_string(int rc);

#endif
