#include "hmmer/daemon.h"
#include "cwd.h"
#include "die.h"
#include "fs.h"
#include "hmmer/name.h"
#include "hmmer/port.h"
#include "hmmer/state.h"
#include "logy.h"
#include "loop/child.h"
#include "loop/global.h"
#include "loop/now.h"
#include "loop/sleep.h"
#include "podman.h"
#include "stringify.h"
#include "unused.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <uv.h>

static enum hmmerd_state state = HMMERD_OFF;

static char exepath[FILENAME_MAX] = "";
static char volume[FILENAME_MAX * 2] = "";
static char hmmfile[FILENAME_MAX] = "";

static char const *argv[] = {exepath,
                             "run",
                             "-t",
                             "-p",
                             stringify(HMMER_PORT) ":" stringify(HMMER_PORT),
                             "--arch=amd64",
                             "-v",
                             volume,
                             "--name",
                             CONTAINER_NAME,
                             "--rm",
                             "quay.io/microbiome-informatics/h3daemon",
                             hmmfile,
                             NULL};

static struct child *child = NULL;

static void on_read(char *line);
static void on_eof(void);
static void on_error(void);
static void on_exit(int, void *);

enum boot_state
{
    INIT,
    STOP_CONTAINER,
    RM_CONTAINER,
    START_CONTAINER,
};

static void start_container(void)
{
    strcpy(exepath, podman_exe());

    strcpy(volume, cwd());
    strcat(volume, "/");
    strcat(volume, hmmfile);
    strcat(volume, ":/app/data/");
    strcat(volume, hmmfile);

    child = child_new();
    if (!child)
    {
        enomem("could not alloc child");
        state = HMMERD_OFF;
        return;
    }

    // child_enable_stdin(child, &on_error);
    child_enable_stdout(child, &on_read, &on_eof, &on_error);
    child_enable_stderr(child);
    child_set_on_exit(child, &on_exit);
    child_set_callb_arg(child, NULL);
    child_set_auto_delete(child, false);

    if (!child_spawn(child, argv))
    {
        efail("could not spawn child");
        state = HMMERD_OFF;
    }
}

static void boot(int status, void *arg)
{
    unused(status);
    enum boot_state boot_state = (int)(intptr_t)arg;

    if (boot_state == STOP_CONTAINER)
        podman_stop(CONTAINER_NAME, 3, &boot, (void *)(intptr_t)RM_CONTAINER);
    else if (boot_state == RM_CONTAINER)
        podman_rm(CONTAINER_NAME, &boot, (void *)(intptr_t)START_CONTAINER);
    else if (boot_state == START_CONTAINER)
        start_container();
}

int hmmerd_start(char const *hmm)
{
    if (!fs_exists(hmm)) return eio("%s file not found", hmm);

    if (state != HMMERD_OFF) fatal("daemon must be off to start it");
    state = HMMERD_BOOT;

    strcpy(hmmfile, hmm);
    boot(0, (void *)(intptr_t)STOP_CONTAINER);
    return RC_OK;
}

void hmmerd_stop(void)
{
    if (state == HMMERD_OFF) return;
    child_kill(child);
}

int hmmerd_state(void) { return state; }

char const *hmmerd_hmmfile(void) { return hmmfile; }

void hmmerd_close(void)
{
    if (state == HMMERD_OFF) return;
    if (child_closed(child))
        child_del(child);
    else
    {
        child_set_auto_delete(child, true);
        child_close(child);
    }
    child = NULL;
    state = HMMERD_OFF;
}

static void on_read(char *line)
{
    if (state == HMMERD_ON) return;
    static char const ready[] = "Pending worker 127.0.0.1";
    if (!strncmp(ready, line, sizeof ready - 1)) state = HMMERD_ON;
}

static void on_eof(void) {}

static void on_error(void) { child_kill(child); }

static void on_exit(int exit_status, void *arg)
{
    (void)exit_status;
    (void)arg;
    state = HMMERD_OFF;
}
