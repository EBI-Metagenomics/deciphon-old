#ifndef HMMY_SERVER_H
#define HMMY_SERVER_H

#include "state.h"

void server_init(char const *podman_file);
void server_start(char const *hmm_file);
void server_stop(void);
enum state server_state(void);
char const *server_hmmfile(void);
void server_cleanup(void);

#endif
