#ifndef DCP_LOG_H
#define DCP_LOG_H

#include "dcp/export.h"

typedef void dcp_log_print_t(char const *msg, void *arg);

DCP_API void dcp_log_setup(dcp_log_print_t *print, void *arg);

#endif
