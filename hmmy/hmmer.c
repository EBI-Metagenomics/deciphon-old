#include "hmmer.h"
#include "boot.h"
#include "client.h"
#include "server.h"

static enum hmmer_state state = HMMER_OFF;

void hmmer_init(char const *podman)
{
    state = HMMER_OFF;
    server_init(podman);
    client_init();
    boot_init();
}

static void boot_end(bool succeed)
{
    state = succeed ? HMMER_READY : HMMER_FAIL;
}

void hmmer_start(char const *hmm_file)
{
    boot_start(hmm_file, &boot_end);
    state = HMMER_BOOT;
}

void hmmer_stop(void)
{
    boot_stop();
    client_stop();
    server_stop();
}

bool hmmer_offline(void)
{
    return boot_offline() && server_offline() && client_offline();
}

enum hmmer_state hmmer_state(void) { return state; }

char const *hmmer_hmmfile(void) { return server_hmmfile(); }

void hmmer_cleanup(void)
{
    boot_cleanup();
    client_cleanup();
    server_cleanup();
}
