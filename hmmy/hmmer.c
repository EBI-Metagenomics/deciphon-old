#include "hmmer.h"
#include "client.h"
#include "core/fmt.h"
#include "core/global.h"
#include "core/limits.h"
#include "core/logy.h"
#include "loop/proc.h"
#include "server.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

void hmmer_init(void)
{
    server_init("/opt/homebrew/bin/podman");
    client_init(global_exedir());
}

void hmmer_start(char const *hmm_file) {}

void hmmer_stop(void) {}

void hmmer_reset(void) {}

bool hmmer_is_running(void) { return false; }

bool hmmer_is_done(void) { return false; }

char const *hmmer_filename(void) { return ""; }

int hmmer_cancel(int timeout_msec) { return 0; }

char const *hmmer_state_string(void) {}
