#ifndef LOGGER_H
#define LOGGER_H

#include "macros.h"
#include "rc.h"

typedef void dcp_log_print_t(char const *msg, void *arg);
void dcp_log_setup(dcp_log_print_t *print, void *arg);

#define __WARN_FMT(rc, msg) LOCAL(__LINE__) ":" #rc ": " msg
enum dcp_rc __logger_warn(enum dcp_rc rc, char const *msg);
#define warn(rc, msg) __logger_warn(rc, __WARN_FMT(rc, msg))

#define __INFO_FMT(msg) LOCAL(__LINE__) ":" msg
void __logger_info(char const *msg);
#define info(msg) __logger_info(__INFO_FMT(msg))

#define __ERROR_FMT(rc, msg) LOCAL(__LINE__) ":" #rc ": " msg
enum dcp_rc __logger_error(enum dcp_rc rc, char const *msg);
#define error(rc, msg) __logger_error(rc, __ERROR_FMT(rc, msg))

#endif
