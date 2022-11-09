#include "client.h"
#include "core/global.h"
#include "core/logy.h"
#include "loop/child.h"
#include <stdio.h>
#include <string.h>

enum
{
    TIMEOUT = 2000,
};

static struct child child = {0};
static enum state state = {0};
static long time_of_start = 0;

static void on_read(char *line);
static void on_eof(void) {}
static void on_error(void) { child_kill(&child); }
static void on_exit(void);

static char exepath[FILENAME_MAX] = "";

static char const *argv[] = {exepath, NULL};

void client_init(void)
{
    strcpy(exepath, global_exedir());
    strcat(exepath, "/h3client");
}

void client_start(void)
{
    if (state != OFF && state != FAIL) client_stop();
    time_of_start = global_now();

    child_init(&child, &on_read, &on_eof, &on_error, &on_exit);
    child_spawn(&child, argv);
    state = BOOT;
}

void client_stop(void) { child_kill(&child); }

enum state client_state(void)
{
    if (state == BOOT && global_now() - time_of_start >= TIMEOUT)
    {
        state = FAIL;
        client_stop();
    }
    return state;
}

void client_cleanup(void) { child_kill(&child); }

static void on_read(char *line)
{
    static char const ready[] = "ready";
    if (!strncmp(ready, line, sizeof ready - 1))
    {
        info("h3client is online");
        state = ON;
    }
}

static void on_exit(void)
{
    if (child_exit_status(&child))
        state = FAIL;
    else
        state = OFF;
}
