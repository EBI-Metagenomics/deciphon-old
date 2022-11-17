#ifndef LOOP_CALLBACKS_H
#define LOOP_CALLBACKS_H

#include <stdbool.h>

typedef void on_close_fn_t(void *arg);
typedef void on_eof_fn_t(void *arg);
typedef void on_error_fn_t(void *arg);
typedef void on_open_fn_t(bool ok, void *arg);
typedef void on_read_fn_t(char *line, void *arg);
typedef void on_exit_fn_t(void);
typedef void on_exit2_fn_t(void *arg);
typedef void on_exit3_fn_t(int exit_code);
typedef void on_exit4_fn_t(int exit_code, void *arg);

typedef void on_eof2_fn_t(void);
typedef void on_error2_fn_t(void);
typedef void on_read2_fn_t(char *line);
typedef void on_read3_fn_t(char *line, void *arg);

#endif
