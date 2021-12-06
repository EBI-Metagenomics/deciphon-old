#ifndef LOGGER_H
#define LOGGER_H

#include "compiler.h"
#include "rc.h"

typedef void logger_print_t(char const *msg, void *arg);
void logger_setup(logger_print_t *print, void *arg);

#define __WARN_FMT(rc, msg) LOCAL(__LINE__) ":" #rc ": " msg
enum rc __logger_warn(enum rc rc, char const *msg);
#define warn(rc, msg) __logger_warn(rc, __WARN_FMT(rc, msg))

#define __INFO_FMT(msg) LOCAL(__LINE__) ":" msg
void __logger_info(char const *msg);
#define info(msg) __logger_info(__INFO_FMT(msg))

#define __ERROR_FMT(rc, msg) LOCAL(__LINE__) ":" #rc ": " msg
enum rc __logger_error(enum rc rc, char const *msg);
#define error(rc, msg) __logger_error(rc, __ERROR_FMT(rc, msg))

#endif
