#ifndef RC_H
#define RC_H

#include "common/export.h"

enum rc
{
    RC_DONE,
    RC_END,
    RC_NEXT,
    RC_NOTFOUND,
    RC_EFAIL,
    RC_EINVAL,
    RC_EIO,
    RC_ENOMEM,
    RC_EPARSE,
};

EXPORT enum rc rc_from_str(unsigned len, char const *str, enum rc *rc);

#endif
