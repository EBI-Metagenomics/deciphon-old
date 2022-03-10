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
};

enum rc rc_resolve(unsigned len, char const *str, enum rc *rc);

#endif
