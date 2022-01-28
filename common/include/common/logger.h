#ifndef LOGGER_H
#define LOGGER_H

#include "common/compiler.h"
#include "common/export.h"
#include "common/rc.h"

typedef void logger_print_t(char const *msg, void *arg);
EXPORT void logger_setup(logger_print_t *print, void *arg);

#define __WARN_FMT(rc, msg) LOCAL(__LINE__) ":" #rc ": " msg
EXPORT enum rc __logger_warn(enum rc rc, char const *msg);
#define warn(rc, msg) __logger_warn(rc, __WARN_FMT(rc, msg))

// #define __INFO_FMT(msg) LOCAL(__LINE__) ":" msg
// EXPORT void __logger_info(char const *msg);
// #define info(msg) __logger_info(__INFO_FMT(msg))

#define __ERROR_FMT(rc, msg) LOCAL(__LINE__) ":" #rc ": " msg
EXPORT enum rc __logger_error(enum rc rc, char const *msg);
#define error(rc, msg) __logger_error(rc, __ERROR_FMT(rc, msg))

#define eio(what) error(RC_EIO, "failed to " what)
#define efail(what) error(RC_EFAIL, "failed to " what)
#define eparse(what) error(RC_EPARSE, "failed to " what)

#endif
