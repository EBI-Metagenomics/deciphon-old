#include "core/fmt.h"
#include "core/global.h"
#include "core/limits.h"
#include "core/logy.h"
#include "loop/proc.h"
#include "zc.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static void on_read(char *line) { info("%s", line); }
static void on_eof(void) { debug("ON_EOF"); }
static void on_error(void) { debug("ON_ERROR"); }
static void on_exit(void) { debug("ON_EXIT"); }
// static void terminate(void) { debug("TERMINATE"); global_terminate(); }

#define NUM_CONTAINERS 30
#define FIRST_PORT 51371

static int num_containers = 0;
static struct container
{
    struct proc proc;
    char hmm[PATH_SIZE];
    int port;
} containers[NUM_CONTAINERS];

static char proc_file[FILENAME_MAX] = "";

void hmmer_init(void)
{
    strcat(proc_file, global_exedir());
    strcat(proc_file, "/");
    strcat(proc_file, "h3daemon");
}

void hmmer_reset(void) {}

bool hmmer_is_running(void) { return false; }

bool hmmer_is_done(void) { return false; }

static void start_container(struct container *c, char const *hmm, int port)
{
    static char name_arg[sizeof("--name=") + PATH_SIZE] = "";
    static char port_arg[sizeof("--port=") + sizeof("65535")] = "";
    static char const *args[] = {proc_file, "start", NULL, name_arg,
                                 port_arg,  "--yes", NULL};

    strcpy(c->hmm, hmm);
    c->port = port;

    args[2] = c->hmm;

    strcpy(name_arg, "--name=");
    strcat(name_arg, c->hmm);

    strcpy(port_arg, "--port=");
    strcat(port_arg, fmt("%d", c->port));

    proc_init(&c->proc, PROC_CHILD);
    proc_setup(&c->proc, &on_read, &on_eof, &on_error, &on_exit);
    proc_start(&c->proc, args);
}

static struct container *find_container(char const *hmm)
{
    for (int i = 0; i < num_containers; ++i)
    {
        if (!strcmp(containers[i].hmm, hmm)) return containers + i;
    }
    return NULL;
}

static void stop_container(struct container *c)
{
    static char name_arg[sizeof("--name=") + PATH_SIZE] = "";
    static char const *args[] = {proc_file, "stop", name_arg, NULL};

    strcpy(name_arg, "--name=");
    strcat(name_arg, c->hmm);

    info("%s %s %s", args[0], args[1], args[2]);
    proc_init(&c->proc, PROC_CHILD);
    proc_setup(&c->proc, &on_read, &on_eof, &on_error, &on_exit);
    proc_start(&c->proc, args);
}

bool hmmer_start(char const *filename)
{
    if (num_containers >= NUM_CONTAINERS) return false;
    if (find_container(filename)) return false;
    int port = FIRST_PORT + num_containers;
    start_container(containers + num_containers, filename, port);
    ++num_containers;
    return true;
}

bool hmmer_stop(char const *filename)
{
    struct container *c = find_container(filename);
    if (!c) return true;
    stop_container(c);
    return true;
}

char const *hmmer_filename(void) { return ""; }

int hmmer_cancel(int timeout_msec) { return 0; }

char const *hmmer_state_string(void) {}
