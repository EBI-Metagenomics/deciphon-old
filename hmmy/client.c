#include "client.h"
#include "core/logy.h"
#include "loop/child.h"
#include <stdio.h>
#include <string.h>

static struct child child = {0};

static void on_read(char *line);
static void on_eof(void);
static void on_error(void);
static void on_exit(void);

static char exepath[FILENAME_MAX] = "";

static char const *argv[] = {exepath, NULL};

void client_init(char const *exedir)
{
    strcpy(exepath, exedir);
    strcpy(exepath, "/h3client");
}

void client_start(void)
{
    child_init(&child, &on_read, &on_eof, &on_error, &on_exit);
    child_spawn(&child, argv);
}

int client_state(void) { return CLIENT_OFF; }

void client_stop(void) { child_kill(&child); }

static void on_read(char *line) { debug("%s: %s", __FUNCTION__, line); }

static void on_eof(void) { debug("%s", __FUNCTION__); }

static void on_error(void) { debug("%s", __FUNCTION__); }

static void on_exit(void) { debug("%s", __FUNCTION__); }
