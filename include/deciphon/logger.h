#ifndef DECIPHON_LOGGER_H
#define DECIPHON_LOGGER_H

#include "deciphon/bug.h"
#include "deciphon/compiler.h"
#include "deciphon/rc.h"
#include <stdbool.h>

/* No assumption about `msg` lifetime is made. */
typedef void (*logger_print_func_t)(char const *ctx, char const *msg,
                                    void *arg);
/* It assumes that `msg` is of static storage class. */
typedef void (*logger_print_static_func_t)(char const *ctx, char const *msg,
                                           void *arg);

void logger_setup(logger_print_func_t, logger_print_static_func_t, void *arg);

#define error(rc, msg)                                                         \
    BUILD_SWITCH(IS_LITERAL_STRING(msg), __logger_error_static,                \
                 __logger_error)                                               \
    (rc, LOCAL, msg)

#define efail(msg) error(RC_EFAIL, msg)
#define einval(msg) error(RC_EINVAL, msg)
#define eio(msg) error(RC_EIO, msg)
#define enomem(msg) error(RC_ENOMEM, msg)
#define eparse(msg) error(RC_EPARSE, msg)

void info(char const *fmt, ...);

/*
 * Private functions. They are not supposed to be used directly outside this
 * module.
 */

/* No assumption about `msg` lifetime is made. */
enum rc __logger_error(enum rc rc, char const *ctx, char const *msg);
/* It assumes that `msg` is of static storage class. `ctx` is not used. */
enum rc __logger_error_static(enum rc rc, char const *ctx, char const *msg);

#endif
