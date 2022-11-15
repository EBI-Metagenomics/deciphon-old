#include "server.h"
#include "core/global.h"
#include "core/logy.h"
#include "loop/child.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static struct child child = {0};
static enum state state = OFF;
static long deadline = 0;

static void on_read(char *line);
static void on_eof(void) {}
static void on_error(void) { child_kill(&child); }
static void on_exit(void);

static char exepath[FILENAME_MAX] = "";
static char volume[FILENAME_MAX * 2] = "";
static char hmmfile[FILENAME_MAX] = "";

static char const *argv[] = {exepath,
                             "run",
                             "-t",
                             "-p",
                             "51371:51371",
                             "--arch=amd64",
                             "-v",
                             volume,
                             "--health-cmd=/app/bin/check-health",
                             "--health-interval=10s",
                             "--health-start-period=3s",
                             "--health-timeout=2s",
                             "--name",
                             "h3daemon",
                             "--rm",
                             "quay.io/microbiome-informatics/h3daemon",
                             hmmfile,
                             NULL};

void server_init(char const *podman_file)
{
    strcpy(exepath, podman_file);
    state = OFF;
    child_init(&child, &on_read, &on_eof, &on_error, &on_exit);
}

void server_start(char const *hmm_file)
{
    if (!(state == OFF || state == FAIL)) fatal("must be offline to start it");

    strcpy(volume, global_exedir());
    strcat(volume, "/");
    strcat(volume, hmm_file);
    strcat(volume, ":/app/data/");
    strcat(volume, hmm_file);
    strcpy(hmmfile, hmm_file);

    state = BOOT;
    deadline = global_now() + 5000;
    child_spawn(&child, argv);
}

void server_cancel(void)
{
    if (state == OFF || state == CANCEL || state == FAIL) return;
    state = CANCEL;
    deadline = global_now() + 3000;
    child_kill(&child);
}

// bool server_offline(void) { return child_offline(&child); }

enum state server_state(void)
{
    if (state == BOOT && global_now() > deadline)
    {
        state = FAIL;
        server_cancel();
    }
    else if (state == CANCEL && global_now() > deadline)
    {
        state = FAIL;
        server_cancel();
    }
    return state;
}

char const *server_hmmfile(void) { return hmmfile; }

void server_cleanup(void) { child_cleanup(&child); }

static void on_read(char *line)
{
    if (state == CANCEL) return;

    static char const ready[] = "Handling worker 127.0.0.1";
    if (!strncmp(ready, line, sizeof ready - 1))
    {
        info("h3daemon is online");
        state = ON;
    }
}

static void on_exit(void)
{
    assert(state != OFF && state != BOOT);
    if (state == ON || state == CANCEL)
    {
        if (child_exit_status(&child))
            state = FAIL;
        else
            state = OFF;
    }
}
