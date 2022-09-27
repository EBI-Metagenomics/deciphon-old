#ifndef LOOP_LOOPIO_H
#define LOOP_LOOPIO_H

#include "loop/fio.h"
#include "loop/reader.h"
#include "loop/writer.h"

typedef void loopio_onread_fn_t(char *line, void *arg);

struct looper;

struct loopio
{
    struct looper *looper;
    struct fio input;
    struct fio output;
    struct reader reader;
    struct writer writer;
    loopio_onread_fn_t *onread_cb;
    void *arg;
    bool reader_noclose;
    bool writer_noclose;
};

void loopio_init(struct loopio *, struct looper *, loopio_onread_fn_t *,
                 void *arg);
void loopio_open(struct loopio *, char const *input, char const *output);
void loopio_put(struct loopio *, char const *msg);
void loopio_terminate(struct loopio *);

#endif
