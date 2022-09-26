#ifndef LOOP_IPC_H
#define LOOP_IPC_H

#include "loop/io.h"
#include "loop/reader.h"
#include "loop/writer.h"

typedef void ipc_onread_fn_t(char *line, void *arg);

struct uv_loop_s;

struct ipc
{
    struct uv_loop_s *loop;
    struct reader reader;
    struct writer writer;
    ipc_onread_fn_t *onread_cb;
    void *arg;
    bool reader_noclose;
    bool writer_noclose;
    struct uv_stdio_container_s stdio[3];
};

void ipc_init(struct ipc *, struct uv_loop_s *, ipc_onread_fn_t *, void *arg);
void ipc_ifopen(struct ipc *, char const *file, int mode);
void ipc_ofopen(struct ipc *, char const *file, int mode);
void ipc_put(struct ipc *, char const *msg);
void ipc_terminate(struct ipc *);

#endif
