#ifndef LOOP_LOOPIO_H
#define LOOP_LOOPIO_H

#include "loop/fio.h"
#include "loop/reader.h"
#include "loop/writer.h"

typedef void loopio_onread_fn_t(char *line, void *arg);
typedef void loopio_onerror_fn_t(void *arg);

struct loopio
{
    struct uv_loop_s *loop;
    struct fio input;
    struct fio output;
    struct reader reader;
    struct writer writer;
    loopio_onread_fn_t *onread_cb;
    loopio_onerror_fn_t *onerror_cb;
    void *arg;
    bool reader_noclose;
    bool writer_noclose;
};

void loopio_init(struct loopio *, struct uv_loop_s *, loopio_onread_fn_t *,
                 loopio_onerror_fn_t *, void *arg);
void loopio_open(struct loopio *, char const *input, char const *output);
void loopio_put(struct loopio *, char const *msg);
void loopio_terminate(struct loopio *);

#endif
