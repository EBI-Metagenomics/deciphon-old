#include "cfg.h"
#include "core/fmt.h"
#include "core/logy.h"
#include "dotenv.h"
#include "zc.h"
#include <errno.h>
#include <stdlib.h>

static int nthreads = 4;
static char const *host = "127.0.0.1";
static int port = 49329;
static char const *prefix = "";
static char const *key = "change-me";
static char uri[2048] = "http://127.0.0.1:49329";

void cfg_init(void)
{
    if (dotenv_load(".", true)) info("config file not found");
    char const *str = NULL;

    if ((str = getenv("DCP_NTHREADS")))
    {
        nthreads = zc_strto_int(str, NULL, 10);
        if (errno) fatal("failed to convert DCP_NTHREADS to a number");
    }

    if ((str = getenv("DCP_API_HOST"))) host = str;

    if ((str = getenv("DCP_API_PORT")))
    {
        port = zc_strto_int(str, NULL, 10);
        if (errno) fatal("failed to convert DCP_API_PORT to a number");
    }

    if ((str = getenv("DCP_API_PREFIX"))) prefix = str;
    if ((str = getenv("DCP_API_KEY"))) key = str;

    zc_strlcpy(uri, fmt("http://%s:%d", cfg_host(), cfg_port()), sizeof uri);
    zc_strlcat(uri, prefix, sizeof uri);
}

int cfg_nthreads(void) { return nthreads; }

char const *cfg_host(void) { return host; }

int cfg_port(void) { return port; }

char const *cfg_prefix(void) { return prefix; }

char const *cfg_uri(void) { return uri; }

char const *cfg_key(void) { return key; }
