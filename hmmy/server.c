#include "server.h"
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

void server_init(char const *podman_file) { strcpy(exepath, podman_file); }

void server_start(char const *hmm_file)
{
    strcpy(volume, "/Users/horta/code/hmmy/");
    strcat(volume, hmm_file);
    strcat(volume, ":/app/data/");
    strcat(volume, hmm_file);
    strcpy(hmmfile, hmm_file);

    child_init(&child, &on_read, &on_eof, &on_error, &on_exit);
    child_spawn(&child, argv);
}

int server_state(void) { return SERVER_OFF; }

void server_cleanup(void) { child_kill(&child); }

static void on_read(char *line) { debug("%s: %s", __FUNCTION__, line); }

static void on_eof(void) { debug("%s", __FUNCTION__); }

static void on_error(void) { debug("%s", __FUNCTION__); }

static void on_exit(void) { debug("%s", __FUNCTION__); }
