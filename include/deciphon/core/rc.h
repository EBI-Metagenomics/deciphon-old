#ifndef DECIPHON_RC_H
#define DECIPHON_RC_H

enum rc
{
    RC_OK,
    RC_END,
    RC_EFAIL,
    RC_EINVAL,
    RC_EIO,
    RC_ENOMEM,
    RC_EPARSE,
    RC_EREST,
    RC_EHTTP,
};

#define RC_STRING(rc)                                                          \
    rc == RC_OK       ? "RC_OK"                                                \
    : rc == RC_END    ? "RC_END"                                               \
    : rc == RC_EFAIL  ? "RC_EFAIL"                                             \
    : rc == RC_EINVAL ? "RC_EINVAL"                                            \
    : rc == RC_EIO    ? "RC_EIO"                                               \
    : rc == RC_ENOMEM ? "RC_ENOMEM"                                            \
    : rc == RC_EPARSE ? "RC_EPARSE"                                            \
    : rc == RC_EREST  ? "RC_EREST"                                             \
    : rc == RC_EHTTP  ? "RC_EHTTP"                                             \
                      : "invalid return code"

#endif
