#ifndef DECIPHON_SERVER_SERVER_H
#define DECIPHON_SERVER_SERVER_H

#include "imm/imm.h"

enum rc server_run(bool single_run, unsigned num_threads, char const *url);
void server_set_lrt_threshold(imm_float lrt);

#endif
