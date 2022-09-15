#ifndef DECIPHON_SERVER_SERVER_H
#define DECIPHON_SERVER_SERVER_H

#include "imm/imm.h"

struct server_cfg
{
    unsigned num_threads;
    double lrt_threshold;
    unsigned polling_rate;
    bool single_run;
    char api_key[256];
};

#define SERVER_CFG_INIT                                                        \
    (struct server_cfg) { 1, 1.0f, 2, false }

enum rc server_init(char const *sched_api_url, struct server_cfg cfg);
enum rc server_run(void);
void server_cleanup(void);

#endif
