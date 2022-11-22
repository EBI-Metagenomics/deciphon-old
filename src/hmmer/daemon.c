#include "hmmer/daemon.h"
#include "fs.h"
#include "hmmer/state.h"
#include "logy.h"
#include "loop/child.h"
#include "loop/exe.h"
#include "loop/global.h"
#include "loop/now.h"
#include "loop/sleep.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static enum hmmerd_state state = HMMERD_OFF;

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

static struct child *child = NULL;
static long deadline = 0;

static void on_read(char *line);
static void on_eof(void);
static void on_error(void);
static void on_exit(int, void *);

static char const *find_podman(void);

int hmmerd_init(void)
{
    char const *podman = find_podman();
    if (!podman) return efail("failed to find podman");

    strcpy(exepath, podman);
    state = HMMERD_OFF;
    return RC_OK;
}

int hmmerd_start(char const *hmm)
{
    if (state != HMMERD_OFF) fatal("daemon must be off to start it");

    strcpy(volume, exe_cwd());
    strcat(volume, "/");
    strcat(volume, hmm);
    strcat(volume, ":/app/data/");
    strcat(volume, hmm);
    strcpy(hmmfile, hmm);

    deadline = now() + 5000;

    child = child_new(&on_read, &on_eof, &on_error, &on_exit, NULL);
    if (!child) return efail("failed to alloc/init child");

    state = HMMERD_BOOT;
    child_spawn(child, argv);

    return RC_OK;
}

void hmmerd_stop(void)
{
    if (state == HMMERD_OFF) return;
    deadline = now() + 3000;
    child_kill(child);
}

int hmmerd_state(void) { return state; }

void hmmerd_close(void)
{
    if (state == HMMERD_OFF) return;
    child_close(child);
    child_del(child);
    child = NULL;
    deadline = 0;
    state = HMMERD_OFF;
}

static void on_read(char *line)
{
    if (state == HMMERD_ON) return;
    static char const ready[] = "Handling worker 127.0.0.1";
    if (!strncmp(ready, line, sizeof ready - 1))
    {
        info("h3daemon is online");
        state = HMMERD_ON;
    }
}

static void on_eof(void) {}

static void on_error(void) { child_kill(child); }

static void on_exit(int exit_status, void *arg)
{
    (void)exit_status;
    (void)arg;
    state = HMMERD_OFF;
}

static char const *find_podman(void)
{
    static char const *paths[] = {"/usr/bin/podman", "/opt/homebrew/bin/podman",
                                  NULL};
    char const **p = &paths[0];
    while (p)
    {
        if (fs_exists(*p)) return *p;
        ++p;
    }

    return NULL;
}
