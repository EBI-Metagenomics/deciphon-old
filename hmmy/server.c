#include "server.h"
#include "core/global.h"
#include "core/logy.h"
#include "loop/child.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

enum
{
    TIMEOUT = 5000,
};

static struct child child = {0};
static enum state state = OFF;
static long time_of_start = 0;

static void on_read(char *line);
static void on_eof(void) {}
static void on_error(void) { child_stop(&child); }
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
}

void server_start(char const *hmm_file)
{
    if (state != OFF && state != FAIL) server_stop();
    time_of_start = global_now();

    strcpy(volume, global_exedir());
    strcat(volume, "/");
    strcat(volume, hmm_file);
    strcat(volume, ":/app/data/");
    strcat(volume, hmm_file);
    strcpy(hmmfile, hmm_file);

    child_init(&child, &on_read, &on_eof, &on_error, &on_exit);
    child_start(&child, argv);
    state = BOOT;
}

void server_stop(void) { child_stop(&child); }

enum state server_state(void)
{
    if (state == BOOT && global_now() - time_of_start >= TIMEOUT)
    {
        state = FAIL;
        server_stop();
    }
    return state;
}

char const *server_hmmfile(void) { return hmmfile; }

void server_cleanup(void) { child_stop(&child); }

static void on_read(char *line)
{
    static char const ready[] = "Handling worker 127.0.0.1";
    if (!strncmp(ready, line, sizeof ready - 1))
    {
        info("h3daemon is online");
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
