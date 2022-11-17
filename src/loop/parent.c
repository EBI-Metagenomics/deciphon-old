#include "loop/parent.h"
#include "loop/reader.h"
#include "loop/stdpipes.h"
#include "loop/writer.h"
#include <stddef.h>
#include <unistd.h>

static struct reader in = {0};
static struct writer out = {0};
static bool closed = true;

static void on_exit_fwd(void);

void parent_init(on_read2_fn_t *on_read, on_eof2_fn_t *on_eof,
                 on_error2_fn_t *on_error)
{
    stdpipes_init(&on_exit_fwd);
    reader_init(&in, stdpipes_uv(STDIN_FILENO), on_read, on_eof, on_error);
    writer_init(&out, stdpipes_uv(STDOUT_FILENO), on_error);
    closed = false;
    reader_open(&in);
    writer_open(&out);
}

void parent_send(char const *string)
{
    if (string) writer_put(&out, string);
}

void parent_close(void)
{
    reader_close(&in);
    writer_close(&out);
    stdpipes_close();
}

bool parent_closed(void) { return closed; }

static void on_exit_fwd(void) { closed = true; }
