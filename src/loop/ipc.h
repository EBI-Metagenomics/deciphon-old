#ifndef LOOP_IPC_H
#define LOOP_IPC_H

#include "loop/reader.h"
#include "loop/writer.h"

typedef void ipc_onread_fn_t(char *line, void *arg);
typedef void ipc_oneof_fn_t(void *arg);
typedef void ipc_onerror_fn_t(void *arg);
typedef void ipc_onterm_fn_t(void *arg);

struct uv_loop_s;

struct ipc
{
    struct uv_loop_s *loop;
    struct reader reader;
    struct writer writer;
    ipc_onread_fn_t *onread_cb;
    ipc_oneof_fn_t *oneof_cb;
    ipc_onerror_fn_t *onerror_cb;
    ipc_onterm_fn_t *onterm_cb;
    void *arg;
    struct uv_stdio_container_s stdio[3];
};

void ipc_init(struct ipc *, struct uv_loop_s *, ipc_onread_fn_t *,
              ipc_oneof_fn_t *, ipc_onerror_fn_t *, ipc_onterm_fn_t *,
              void *arg);
void ipc_put(struct ipc *, char const *msg);
struct uv_stdio_container_s *ipc_stdio(struct ipc *);
int ipc_stdio_count(struct ipc const *);
void ipc_start_reading(struct ipc *);
void ipc_terminate(struct ipc *);

#endif
