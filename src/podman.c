#include "fs.h"
#include "itoa.h"
#include "logy.h"
#include "loop/child.h"
#include "unused.h"
#include <assert.h>
#include <stddef.h>

static char const *podman = NULL;
static char const *paths[] = {"/usr/bin/podman", "/opt/homebrew/bin/podman",
                              NULL};

void podman_init(void)
{
    if (podman) return;

    char const **p = &paths[0];
    while (p)
    {
        if (fs_exists(*p))
        {
            podman = *p;
            break;
        }
        ++p;
    }

    if (!podman) fatal("podman executable not found");
}

static void assert_init(void);

char const *podman_exe(void)
{
    assert_init();
    return podman;
}

int exec(char const *argv[], void (*callb)(int, void *), void *arg);

int podman_stop(char const *name, int secs, void (*callb)(int, void *),
                void *arg)
{
    podman_init();

    char asecs[ITOA_SIZE] = {0};
    itoa(asecs, secs);

    char const *argv[] = {podman, "container", "stop", "-t", asecs, name, NULL};
    return exec(argv, callb, arg);
}

int podman_rm(char const *name, void (*callb)(int, void *), void *arg)
{
    podman_init();

    char const *argv[] = {podman, "container", "rm", "-f", name, NULL};
    return exec(argv, callb, arg);
}

static void assert_init(void)
{
    if (!podman) fatal("podman module has not been initialized");
}

int exec(char const *argv[], void (*callb)(int, void *), void *arg)
{
    assert_init();

    struct child *child = child_new();
    if (!child) return enomem("could not alloc child");

    child_set_on_exit(child, callb);
    child_set_callb_arg(child, arg);
    child_set_auto_delete(child, true);

    if (!child_spawn(child, argv))
    {
        child_del(child);
        return efail("could not spawn podman process");
    }

    return RC_OK;
}
