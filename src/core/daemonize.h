#ifndef CORE_DAEMONIZE_H
#define CORE_DAEMONIZE_H

#include <stdbool.h>

void daemonize(bool sanitize_stdin, bool sanitize_stdout, bool sanitize_stderr,
               bool close_nonstd_fds);

#endif
