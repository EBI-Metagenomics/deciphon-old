#ifndef DECIPHON_UTIL_LOGGER_H
#define DECIPHON_UTIL_LOGGER_H

#include "deciphon/compiler.h"
#include "deciphon/rc.h"

typedef void logger_print_t(char const *msg, void *arg);
void logger_setup(logger_print_t *print, void *arg);

#define __ERROR_FMT(rc, msg) LOCAL(__LINE__) ":" #rc ": " msg
enum rc __logger_error(enum rc rc, char const *msg);
#define error(rc, msg) __logger_error(rc, __ERROR_FMT(rc, msg))

#define efail(msg) error(RC_EFAIL, msg)
#define einval(msg) error(RC_EINVAL, msg)
#define eio(msg) error(RC_EIO, msg)
#define enomem(msg) error(RC_ENOMEM, msg)
#define eparse(msg) error(RC_EPARSE, msg)

void info(char const *fmt, ...);

#endif
