#include "deciphon/core/liner.h"
#include "deciphon/core/logging.h"
#include "deciphon/core/looper.h"
#include <assert.h>
#include <stdbool.h>
#include <string.h>

static void ioerror_cb(void);
static void newline_cb(char const *line);
static void onterm_cb(void);

static struct looper looper = {0};
static struct liner liner = {0};

int main(void)
{
    looper_init(&looper, onterm_cb);
    liner_init(&liner, &looper, ioerror_cb, newline_cb);
    liner_open(&liner, 0);

    looper_run(&looper);

    looper_cleanup(&looper);
}

static void ioerror_cb(void)
{
    fputs("IO ERROR\n", stderr);
    looper_terminate(&looper);
}

static void newline_cb(char const *line)
{
    puts(line);
    fflush(stdout);
}

static void onterm_cb(void) { liner_close(&liner); }
